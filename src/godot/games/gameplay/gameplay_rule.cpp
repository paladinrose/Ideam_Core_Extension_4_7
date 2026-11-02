#include "gameplay_rule.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "../game.h"
#include "../game_entity.h"
#include "../game_entities/actions/game_agent_action.h"
#include "../game_entities/actions/game_interaction.h"
#include "../game_entities/game_agent.h"
#include "../game_entities/game_piece.h"
#include "../game_property.h"
#include "../game_property_modifier.h"

namespace ideam::godot_ext {

void GameplayRule::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(RuleScope::SELF);
    BIND_ENUM_CONSTANT(RuleScope::DESCENDANTS);
    BIND_ENUM_CONSTANT(RuleScope::ALL);

    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_BASE);
    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_MODE);
    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_DIFFICULTY);
    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_ENVIRONMENT);
    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_PLAYER_ACCESSIBILITY);
    BIND_ENUM_CONSTANT(RuleCascadeLayer::LAYER_INLINE_LOCAL);

    BIND_ENUM_CONSTANT(RuleOp::SET);
    BIND_ENUM_CONSTANT(RuleOp::ADD);
    BIND_ENUM_CONSTANT(RuleOp::MULTIPLY);
    BIND_ENUM_CONSTANT(RuleOp::MODIFIER_ADD);
    BIND_ENUM_CONSTANT(RuleOp::CLAMP);
    BIND_ENUM_CONSTANT(RuleOp::MODIFIER_REMOVE);
    BIND_ENUM_CONSTANT(RuleOp::GROUP_ADD);
    BIND_ENUM_CONSTANT(RuleOp::GROUP_REMOVE);
    BIND_ENUM_CONSTANT(RuleOp::INTERACTION_WEIGHT);
    BIND_ENUM_CONSTANT(RuleOp::SIGNAL_CONNECT);
    BIND_ENUM_CONSTANT(RuleOp::NODE_RETARGET);

    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_VALUE);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_MAX_VALUE);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_MIN_VALUE);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_LOCKED);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_RESTRICT_MAX);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_RESTRICT_MIN);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_SET);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_MODS);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_MAX_SET);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_MAX_MODS);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_MIN_SET);
    BIND_ENUM_CONSTANT(RulePropertyTarget::PROP_TARGET_ALLOW_MIN_MODS);

    // --- Scope & Cascade Layer ---
    ADD_GROUP("Scope & Layer", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_scope", "scope"), &GameplayRule::set_scope);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scope"), &GameplayRule::get_scope);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "scope", godot::PROPERTY_HINT_ENUM, "Self,Descendants,All"),
        "set_scope",
        "get_scope"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_cascade_layer", "layer"), &GameplayRule::set_cascade_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cascade_layer"), &GameplayRule::get_cascade_layer);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "cascade_layer",
            godot::PROPERTY_HINT_ENUM,
            "Base (Style),Mode,Difficulty,Environment (Board),Player & Accessibility,Inline (Entity)"
        ),
        "set_cascade_layer",
        "get_cascade_layer"
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

    // --- Structural Reflection (PartSignature) Subgroup ---
    ADD_SUBGROUP("Part Signature (Reflection)", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_required_properties", "properties"), &GameplayRule::set_required_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_required_properties"), &GameplayRule::get_required_properties);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "required_properties", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_required_properties",
        "get_required_properties"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_required_methods", "methods"), &GameplayRule::set_required_methods);
    godot::ClassDB::bind_method(godot::D_METHOD("get_required_methods"), &GameplayRule::get_required_methods);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "required_methods", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_required_methods",
        "get_required_methods"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_required_signals", "signals"), &GameplayRule::set_required_signals);
    godot::ClassDB::bind_method(godot::D_METHOD("get_required_signals"), &GameplayRule::get_required_signals);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "required_signals", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_required_signals",
        "get_required_signals"
    );

    // --- Pseudo-State (:state) & Condition Subgroup ---
    ADD_SUBGROUP("Pseudo-State & Condition", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_pseudo_state", "pseudo_state"), &GameplayRule::set_pseudo_state);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pseudo_state"), &GameplayRule::get_pseudo_state);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING_NAME, "pseudo_state"),
        "set_pseudo_state",
        "get_pseudo_state"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_gameplay_condition", "condition"), &GameplayRule::set_gameplay_condition);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gameplay_condition"), &GameplayRule::get_gameplay_condition);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "gameplay_condition", godot::PROPERTY_HINT_NONE, "Incident_Condition"),
        "set_gameplay_condition",
        "get_gameplay_condition"
    );

    // --- Declaration Group ---
    ADD_GROUP("Declaration", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_operation", "operation"), &GameplayRule::set_operation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_operation"), &GameplayRule::get_operation);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "operation",
            godot::PROPERTY_HINT_ENUM,
            "Set,Add,Multiply,Modifier Add,Clamp,Modifier Remove,Group Add,Group Remove,Interaction Weight,Signal Connect,Node Retarget"
        ),
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

    godot::ClassDB::bind_method(godot::D_METHOD("set_clamp_min", "clamp_min"), &GameplayRule::set_clamp_min);
    godot::ClassDB::bind_method(godot::D_METHOD("get_clamp_min"), &GameplayRule::get_clamp_min);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::NIL,
            "clamp_min",
            godot::PROPERTY_HINT_NONE,
            "",
            godot::PROPERTY_USAGE_DEFAULT | godot::PROPERTY_USAGE_NIL_IS_VARIANT
        ),
        "set_clamp_min",
        "get_clamp_min"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_clamp_max", "clamp_max"), &GameplayRule::set_clamp_max);
    godot::ClassDB::bind_method(godot::D_METHOD("get_clamp_max"), &GameplayRule::get_clamp_max);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::NIL,
            "clamp_max",
            godot::PROPERTY_HINT_NONE,
            "",
            godot::PROPERTY_USAGE_DEFAULT | godot::PROPERTY_USAGE_NIL_IS_VARIANT
        ),
        "set_clamp_max",
        "get_clamp_max"
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

    // --- Payload 1: GameProperty Sub-Target & Modifier Subgroup ---
    ADD_SUBGROUP("GameProperty & Modifier Payload", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_property_target", "property_target"), &GameplayRule::set_property_target);
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_target"), &GameplayRule::get_property_target);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "property_target",
            godot::PROPERTY_HINT_ENUM,
            "Value,Max Value,Min Value,Locked,Restrict To Max,Restrict To Min,Allow Value Set,Allow Value Modifiers,Allow Max Set,Allow Max Modifiers,Allow Min Set,Allow Min Modifiers"
        ),
        "set_property_target",
        "get_property_target"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_modifier_type", "modifier_type"), &GameplayRule::set_modifier_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modifier_type"), &GameplayRule::get_modifier_type);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "modifier_type",
            godot::PROPERTY_HINT_ENUM,
            "Static,Timed,Timed Diminishing,Resource"
        ),
        "set_modifier_type",
        "get_modifier_type"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_modifier_time_limit", "modifier_time_limit"), &GameplayRule::set_modifier_time_limit);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modifier_time_limit"), &GameplayRule::get_modifier_time_limit);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "modifier_time_limit", godot::PROPERTY_HINT_RANGE, "0.0,3600.0,0.05"),
        "set_modifier_time_limit",
        "get_modifier_time_limit"
    );

    // --- Payload 3: Action & Interaction Laws Subgroup ---
    ADD_SUBGROUP("Interaction Law Payload", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_instigator_weight", "instigator_weight"), &GameplayRule::set_instigator_weight);
    godot::ClassDB::bind_method(godot::D_METHOD("get_instigator_weight"), &GameplayRule::get_instigator_weight);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "instigator_weight", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.05"),
        "set_instigator_weight",
        "get_instigator_weight"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_competition_weight", "competition_weight"), &GameplayRule::set_competition_weight);
    godot::ClassDB::bind_method(godot::D_METHOD("get_competition_weight"), &GameplayRule::get_competition_weight);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "competition_weight", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.05"),
        "set_competition_weight",
        "get_competition_weight"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_consequence_score_multiplier", "consequence_score_multiplier"), &GameplayRule::set_consequence_score_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_consequence_score_multiplier"), &GameplayRule::get_consequence_score_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "consequence_score_multiplier", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.05"),
        "set_consequence_score_multiplier",
        "get_consequence_score_multiplier"
    );

    // --- Payload 4: Reactive Signal & Node Retarget Hooks Subgroup ---
    ADD_SUBGROUP("Reactive Hooks Payload", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_signal_connector", "signal_connector"), &GameplayRule::set_signal_connector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_signal_connector"), &GameplayRule::get_signal_connector);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "signal_connector", godot::PROPERTY_HINT_RESOURCE_TYPE, "SignalConnector"),
        "set_signal_connector",
        "get_signal_connector"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_node_retargeter", "node_retargeter"), &GameplayRule::set_node_retargeter);
    godot::ClassDB::bind_method(godot::D_METHOD("get_node_retargeter"), &GameplayRule::get_node_retargeter);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "node_retargeter", godot::PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"),
        "set_node_retargeter",
        "get_node_retargeter"
    );

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("applies_to_self"), &GameplayRule::applies_to_self);
    godot::ClassDB::bind_method(godot::D_METHOD("applies_to_descendants"), &GameplayRule::applies_to_descendants);
    godot::ClassDB::bind_method(godot::D_METHOD("get_specificity"), &GameplayRule::get_specificity);
    godot::ClassDB::bind_method(godot::D_METHOD("outweighs", "other"), &GameplayRule::outweighs);
    godot::ClassDB::bind_method(godot::D_METHOD("has_same_target", "other"), &GameplayRule::has_same_target);

    godot::ClassDB::bind_method(godot::D_METHOD("compute_weighted_interaction_score", "instigator_value", "competition_value"), &GameplayRule::compute_weighted_interaction_score);
    godot::ClassDB::bind_method(godot::D_METHOD("scale_consequence_score", "score"), &GameplayRule::scale_consequence_score);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("evaluate_interaction_score_for_object", "interaction", "instigator_value", "competition_value"), &GameplayRule::evaluate_interaction_score_for_object);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("scale_consequence_score_for_object", "interaction", "score"), &GameplayRule::scale_consequence_score_for_object);

    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("cache_base_value", "object", "property", "value"), &GameplayRule::cache_base_value);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("has_cached_base_value", "object", "property"), &GameplayRule::has_cached_base_value);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("get_cached_base_value", "object", "property", "fallback"), &GameplayRule::get_cached_base_value, DEFVAL(godot::Variant()));
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("restore_base_value", "object", "property"), &GameplayRule::restore_base_value);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("restore_all_base_values", "object"), &GameplayRule::restore_all_base_values);
    godot::ClassDB::bind_static_method("GameplayRule", godot::D_METHOD("revert_all_from_object", "object"), &GameplayRule::revert_all_from_object);

    godot::ClassDB::bind_method(godot::D_METHOD("find_target_game_property_on_object", "object"), &GameplayRule::find_target_game_property_on_object);
    godot::ClassDB::bind_method(godot::D_METHOD("find_target_game_property", "entity"), &GameplayRule::find_target_game_property);

    godot::ClassDB::bind_method(godot::D_METHOD("matches_part_signature", "object"), &GameplayRule::matches_part_signature);
    godot::ClassDB::bind_method(godot::D_METHOD("matches_pseudo_state", "object"), &GameplayRule::matches_pseudo_state);

    godot::ClassDB::bind_method(godot::D_METHOD("matches_object", "object", "is_self"), &GameplayRule::matches_object, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("matches_entity", "entity", "is_self"), &GameplayRule::matches_entity, DEFVAL(true));

    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_object", "object", "is_self"), &GameplayRule::apply_to_object, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_entity", "entity", "is_self"), &GameplayRule::apply_to_entity, DEFVAL(true));

    godot::ClassDB::bind_method(godot::D_METHOD("revert_from_object", "object"), &GameplayRule::revert_from_object);
    godot::ClassDB::bind_method(godot::D_METHOD("revert_from_entity", "entity"), &GameplayRule::revert_from_entity);

    godot::ClassDB::bind_method(godot::D_METHOD("save_rule"), &GameplayRule::save_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("load_rule", "data"), &GameplayRule::load_rule);
}

GameplayRule::GameplayRule() {
    // DOD NOTE: Minimalist initialization keeps Resource instantiation fast.
}

GameplayRule::~GameplayRule() {
}

// --- Scope & Layer Setters / Getters ---

void GameplayRule::set_scope(RuleScope p_scope) {
    if (scope == p_scope) return;
    scope = p_scope;
    emit_changed();
}

RuleScope GameplayRule::get_scope() const {
    return scope;
}

void GameplayRule::set_cascade_layer(RuleCascadeLayer p_layer) {
    if (cascade_layer == p_layer) return;
    cascade_layer = p_layer;
    emit_changed();
}

RuleCascadeLayer GameplayRule::get_cascade_layer() const {
    return cascade_layer;
}

bool GameplayRule::applies_to_self() const {
    return scope == RuleScope::SELF || scope == RuleScope::ALL;
}

bool GameplayRule::applies_to_descendants() const {
    return scope == RuleScope::DESCENDANTS || scope == RuleScope::ALL;
}

// --- Core Selector Setters / Getters ---

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

// --- Structural Reflection (PartSignature) Setters / Getters ---

void GameplayRule::set_required_properties(const godot::TypedArray<godot::StringName> &p_properties) {
    if (required_properties == p_properties) return;
    required_properties = p_properties;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayRule::get_required_properties() const {
    return required_properties;
}

void GameplayRule::set_required_methods(const godot::TypedArray<godot::StringName> &p_methods) {
    if (required_methods == p_methods) return;
    required_methods = p_methods;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayRule::get_required_methods() const {
    return required_methods;
}

void GameplayRule::set_required_signals(const godot::TypedArray<godot::StringName> &p_signals) {
    if (required_signals == p_signals) return;
    required_signals = p_signals;
    emit_changed();
}

godot::TypedArray<godot::StringName> GameplayRule::get_required_signals() const {
    return required_signals;
}

// --- Pseudo-State & Condition Setters / Getters ---

void GameplayRule::set_pseudo_state(const godot::StringName &p_pseudo_state) {
    if (pseudo_state == p_pseudo_state) return;
    pseudo_state = p_pseudo_state;
    emit_changed();
}

godot::StringName GameplayRule::get_pseudo_state() const {
    return pseudo_state;
}

void GameplayRule::set_gameplay_condition(const godot::Ref<Incident_Condition> &p_condition) {
    if (gameplay_condition == p_condition) return;
    gameplay_condition = p_condition;
    emit_changed();
}

godot::Ref<Incident_Condition> GameplayRule::get_gameplay_condition() const {
    return gameplay_condition;
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

void GameplayRule::set_clamp_min(const godot::Variant &p_min) {
    if (clamp_min == p_min) return;
    clamp_min = p_min;
    emit_changed();
}

godot::Variant GameplayRule::get_clamp_min() const {
    return clamp_min;
}

void GameplayRule::set_clamp_max(const godot::Variant &p_max) {
    if (clamp_max == p_max) return;
    clamp_max = p_max;
    emit_changed();
}

godot::Variant GameplayRule::get_clamp_max() const {
    return clamp_max;
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

// --- Payload 1: GameProperty Sub-Target & Modifier Setters / Getters ---

void GameplayRule::set_property_target(RulePropertyTarget p_target) {
    if (property_target == p_target) return;
    property_target = p_target;
    emit_changed();
}

RulePropertyTarget GameplayRule::get_property_target() const {
    return property_target;
}

void GameplayRule::set_modifier_type(ModifierType p_type) {
    if (modifier_type == p_type) return;
    modifier_type = p_type;
    emit_changed();
}

ModifierType GameplayRule::get_modifier_type() const {
    return modifier_type;
}

void GameplayRule::set_modifier_time_limit(float p_time_limit) {
    if (godot::Math::is_equal_approx(modifier_time_limit, p_time_limit)) return;
    modifier_time_limit = p_time_limit;
    emit_changed();
}

float GameplayRule::get_modifier_time_limit() const {
    return modifier_time_limit;
}

// --- Payload 3: Action & Interaction Law Setters / Getters ---

void GameplayRule::set_instigator_weight(float p_weight) {
    if (godot::Math::is_equal_approx(instigator_weight, p_weight)) return;
    instigator_weight = p_weight;
    emit_changed();
}

float GameplayRule::get_instigator_weight() const {
    return instigator_weight;
}

void GameplayRule::set_competition_weight(float p_weight) {
    if (godot::Math::is_equal_approx(competition_weight, p_weight)) return;
    competition_weight = p_weight;
    emit_changed();
}

float GameplayRule::get_competition_weight() const {
    return competition_weight;
}

void GameplayRule::set_consequence_score_multiplier(float p_multiplier) {
    if (godot::Math::is_equal_approx(consequence_score_multiplier, p_multiplier)) return;
    consequence_score_multiplier = p_multiplier;
    emit_changed();
}

float GameplayRule::get_consequence_score_multiplier() const {
    return consequence_score_multiplier;
}

int GameplayRule::compute_weighted_interaction_score(int p_instigator_value, int p_competition_value) const {
    double weighted_inst = static_cast<double>(p_instigator_value) * static_cast<double>(instigator_weight);
    double weighted_comp = static_cast<double>(p_competition_value) * static_cast<double>(competition_weight);
    return static_cast<int>(godot::Math::round(weighted_inst - weighted_comp));
}

int GameplayRule::scale_consequence_score(int p_score) const {
    return static_cast<int>(godot::Math::round(static_cast<double>(p_score) * static_cast<double>(consequence_score_multiplier)));
}

int GameplayRule::evaluate_interaction_score_for_object(const godot::Object *p_interaction, int p_instigator_value, int p_competition_value) {
    double inst_w = 1.0;
    double comp_w = 1.0;
    if (p_interaction) {
        if (p_interaction->has_meta(META_INTERACTION_INSTIGATOR_WEIGHT)) {
            inst_w = static_cast<double>(p_interaction->get_meta(META_INTERACTION_INSTIGATOR_WEIGHT));
        }
        if (p_interaction->has_meta(META_INTERACTION_COMPETITION_WEIGHT)) {
            comp_w = static_cast<double>(p_interaction->get_meta(META_INTERACTION_COMPETITION_WEIGHT));
        }
    }
    return static_cast<int>(godot::Math::round(static_cast<double>(p_instigator_value) * inst_w - static_cast<double>(p_competition_value) * comp_w));
}

int GameplayRule::scale_consequence_score_for_object(const godot::Object *p_interaction, int p_score) {
    double mult = 1.0;
    if (p_interaction && p_interaction->has_meta(META_INTERACTION_CONSEQUENCE_MULT)) {
        mult = static_cast<double>(p_interaction->get_meta(META_INTERACTION_CONSEQUENCE_MULT));
    }
    return static_cast<int>(godot::Math::round(static_cast<double>(p_score) * mult));
}

// --- Payload 4: Reactive Signal & Node Retarget Setters / Getters ---

void GameplayRule::set_signal_connector(const godot::Ref<SignalConnector> &p_connector) {
    if (signal_connector == p_connector) return;
    signal_connector = p_connector;
    emit_changed();
}

godot::Ref<SignalConnector> GameplayRule::get_signal_connector() const {
    return signal_connector;
}

void GameplayRule::set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter) {
    if (node_retargeter == p_retargeter) return;
    node_retargeter = p_retargeter;
    emit_changed();
}

godot::Ref<NodeRetargeter> GameplayRule::get_node_retargeter() const {
    return node_retargeter;
}

// --- Specificity & Cascade Precedence ---

int GameplayRule::get_specificity() const {
    // CSS-inspired specificity weighting:
    // - Specific title/group match carries higher weight (+2) than a class-wide match (+1)
    // - Explicit target property adds (+1)
    // - Structural reflection requirements (PartSignature) add (+1) each
    // - Runtime pseudo-state (:state) or narrative condition adds (+2, like CSS :pseudo-class)
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
    score += required_properties.size();
    score += required_methods.size();
    score += required_signals.size();
    if (!pseudo_state.is_empty()) {
        score += 2;
    }
    if (gameplay_condition.is_valid()) {
        score += 2;
    }
    return score;
}

bool GameplayRule::outweighs(const godot::Ref<GameplayRule> &p_other) const {
    if (!p_other.is_valid()) {
        return true;
    }
    // 1. !important overrides normal rules
    if (is_important != p_other->get_is_important()) {
        return is_important;
    }
    // 2. Cascade Layer (@layer / origin precedence: Inline > Accessibility > Environment > Difficulty > Mode > Base)
    if (cascade_layer != p_other->get_cascade_layer()) {
        return static_cast<int>(cascade_layer) > static_cast<int>(p_other->get_cascade_layer());
    }
    // 3. Explicit rule priority
    if (priority != p_other->get_priority()) {
        return priority > p_other->get_priority();
    }
    // 4. Selector specificity (ties won by later/child rule in cascade order)
    return get_specificity() >= p_other->get_specificity();
}

bool GameplayRule::has_same_target(const godot::Ref<GameplayRule> &p_other) const {
    if (!p_other.is_valid()) {
        return false;
    }
    return target_class == p_other->get_target_class() &&
           target_title_or_group == p_other->get_target_title_or_group() &&
           target_property == p_other->get_target_property() &&
           property_target == p_other->get_property_target() &&
           pseudo_state == p_other->get_pseudo_state();
}

// --- Base Value Caching & Restoration ---

void GameplayRule::cache_base_value(godot::Object *p_object, const godot::StringName &p_property, const godot::Variant &p_value) {
    if (!p_object || p_property.is_empty()) {
        return;
    }
    godot::Dictionary base_map;
    if (p_object->has_meta(META_BASE_VALUES)) {
        godot::Variant existing = p_object->get_meta(META_BASE_VALUES);
        if (existing.get_type() == godot::Variant::DICTIONARY) {
            base_map = existing;
        }
    }
    godot::String key(p_property);
    if (!base_map.has(key)) {
        base_map[key] = p_value;
        p_object->set_meta(META_BASE_VALUES, base_map);
    }
}

bool GameplayRule::has_cached_base_value(const godot::Object *p_object, const godot::StringName &p_property) {
    if (!p_object || p_property.is_empty() || !p_object->has_meta(META_BASE_VALUES)) {
        return false;
    }
    godot::Variant existing = p_object->get_meta(META_BASE_VALUES);
    if (existing.get_type() != godot::Variant::DICTIONARY) {
        return false;
    }
    godot::Dictionary base_map = existing;
    return base_map.has(godot::String(p_property));
}

godot::Variant GameplayRule::get_cached_base_value(const godot::Object *p_object, const godot::StringName &p_property, const godot::Variant &p_fallback) {
    if (!has_cached_base_value(p_object, p_property)) {
        return p_fallback;
    }
    godot::Dictionary base_map = p_object->get_meta(META_BASE_VALUES);
    return base_map[godot::String(p_property)];
}

bool GameplayRule::restore_base_value(godot::Object *p_object, const godot::StringName &p_property) {
    if (!has_cached_base_value(p_object, p_property)) {
        return false;
    }
    godot::Dictionary base_map = p_object->get_meta(META_BASE_VALUES);
    godot::String key(p_property);
    godot::Variant base_val = base_map[key];
    p_object->set(p_property, base_val);
    base_map.erase(key);
    p_object->set_meta(META_BASE_VALUES, base_map);
    return true;
}

void GameplayRule::restore_all_base_values(godot::Object *p_object) {
    if (!p_object || !p_object->has_meta(META_BASE_VALUES)) {
        return;
    }
    godot::Variant existing = p_object->get_meta(META_BASE_VALUES);
    if (existing.get_type() == godot::Variant::DICTIONARY) {
        godot::Dictionary base_map = existing;
        godot::Array keys = base_map.keys();
        for (int i = 0; i < keys.size(); ++i) {
            godot::String key = keys[i];
            p_object->set(godot::StringName(key), base_map[key]);
        }
    }
    p_object->remove_meta(META_BASE_VALUES);
}

void GameplayRule::revert_all_from_object(godot::Object *p_object) {
    if (!p_object) {
        return;
    }

    // 1. Restore direct base values on the object itself
    restore_all_base_values(p_object);

    // 2. Clear interaction weight metadata if present
    if (p_object->has_meta(META_INTERACTION_INSTIGATOR_WEIGHT)) {
        p_object->remove_meta(META_INTERACTION_INSTIGATOR_WEIGHT);
    }
    if (p_object->has_meta(META_INTERACTION_COMPETITION_WEIGHT)) {
        p_object->remove_meta(META_INTERACTION_COMPETITION_WEIGHT);
    }
    if (p_object->has_meta(META_INTERACTION_CONSEQUENCE_MULT)) {
        p_object->remove_meta(META_INTERACTION_CONSEQUENCE_MULT);
    }

    // 3. Disconnect active SignalConnectors and clear NodeRetargeters if p_object is a Node
    if (godot::Node *node = godot::Object::cast_to<godot::Node>(p_object)) {
        godot::Node *root = node;
        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(node)) {
            if (ent->get_root_node()) {
                root = ent->get_root_node();
            }
        }
        if (node->has_meta(META_ACTIVE_SIGNAL_CONNECTORS)) {
            godot::Array connectors = node->get_meta(META_ACTIVE_SIGNAL_CONNECTORS);
            for (int i = 0; i < connectors.size(); ++i) {
                if (SignalConnector *sc = godot::Object::cast_to<SignalConnector>(connectors[i])) {
                    sc->disconnect_signals(root, root);
                }
            }
            node->remove_meta(META_ACTIVE_SIGNAL_CONNECTORS);
        }
        if (node->has_meta(META_ACTIVE_NODE_RETARGETERS)) {
            godot::Array retargeters = node->get_meta(META_ACTIVE_NODE_RETARGETERS);
            for (int i = 0; i < retargeters.size(); ++i) {
                if (NodeRetargeter *nr = godot::Object::cast_to<NodeRetargeter>(retargeters[i])) {
                    nr->clear_set_targets(root, root);
                }
            }
            node->remove_meta(META_ACTIVE_NODE_RETARGETERS);
        }

        // 4. Restore base values and remove rule-injected modifiers on child GameProperty nodes
        auto clean_game_prop = [](GameProperty *prop) {
            if (!prop) return;
            restore_all_base_values(prop);
            if (prop->has_meta(META_ACTIVE_MODIFIERS)) {
                godot::Dictionary mod_map = prop->get_meta(META_ACTIVE_MODIFIERS);
                godot::Array keys = mod_map.keys();
                for (int k = 0; k < keys.size(); ++k) {
                    if (GamePropertyModifier *mod = godot::Object::cast_to<GamePropertyModifier>(mod_map[keys[k]])) {
                        prop->remove_modifier(mod);
                        prop->remove_max_modifier(mod);
                        prop->remove_min_modifier(mod);
                        if (mod->get_parent() == prop) {
                            prop->remove_child(mod);
                            mod->queue_free();
                        }
                    }
                }
                prop->remove_meta(META_ACTIVE_MODIFIERS);
            }
        };

        if (GameProperty *self_prop = godot::Object::cast_to<GameProperty>(node)) {
            clean_game_prop(self_prop);
        }
        if (GamePiece *piece = godot::Object::cast_to<GamePiece>(node)) {
            godot::TypedArray<GameProperty> props = piece->get_game_properties();
            for (int i = 0; i < props.size(); ++i) {
                clean_game_prop(godot::Object::cast_to<GameProperty>(props[i]));
            }
        }
        int child_count = node->get_child_count();
        for (int i = 0; i < child_count; ++i) {
            if (GameProperty *child_prop = godot::Object::cast_to<GameProperty>(node->get_child(i))) {
                clean_game_prop(child_prop);
            }
        }
    }
}

// --- Internal Helpers ---

bool GameplayRule::_object_has_direct_property(const godot::Object *p_object, const godot::StringName &p_property) const {
    if (!p_object || p_property.is_empty()) {
        return false;
    }

    godot::TypedArray<godot::Dictionary> prop_list = p_object->get_property_list();
    godot::String prop_str(p_property);
    for (int i = 0; i < prop_list.size(); ++i) {
        godot::Dictionary prop_info = prop_list[i];
        if (prop_info.has("name") && godot::String(prop_info["name"]) == prop_str) {
            return true;
        }
    }
    return false;
}

void GameplayRule::_parse_target_property(godot::StringName &r_base_property, RulePropertyTarget &r_sub_target) const {
    r_base_property = target_property;
    r_sub_target = property_target;

    if (target_property.is_empty()) {
        return;
    }

    godot::String prop_str(target_property);
    int sep_idx = prop_str.find(":");
    if (sep_idx < 0) {
        sep_idx = prop_str.find(".");
    }
    if (sep_idx <= 0) {
        return;
    }

    godot::String base_part = prop_str.substr(0, sep_idx);
    godot::String sub_part = prop_str.substr(sep_idx + 1).to_lower();
    r_base_property = godot::StringName(base_part);

    if (sub_part == "value" || sub_part == "_value") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_VALUE;
    } else if (sub_part == "max_value" || sub_part == "_max_value" || sub_part == "max") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_MAX_VALUE;
    } else if (sub_part == "min_value" || sub_part == "_min_value" || sub_part == "min") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_MIN_VALUE;
    } else if (sub_part == "locked") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_LOCKED;
    } else if (sub_part == "restrict_value_to_current_max_value" || sub_part == "restrict_max") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_RESTRICT_MAX;
    } else if (sub_part == "restrict_value_to_current_min_value" || sub_part == "restrict_min") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_RESTRICT_MIN;
    } else if (sub_part == "allow_value_set") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_SET;
    } else if (sub_part == "allow_value_modifiers") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_MODS;
    } else if (sub_part == "allow_max_value_set") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_MAX_SET;
    } else if (sub_part == "allow_max_modifiers") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_MAX_MODS;
    } else if (sub_part == "allow_min_value_set") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_MIN_SET;
    } else if (sub_part == "allow_min_modifiers") {
        r_sub_target = RulePropertyTarget::PROP_TARGET_ALLOW_MIN_MODS;
    }
}

godot::StringName GameplayRule::_get_modifier_meta_tag() const {
    godot::StringName eff_id = get_effective_id();
    if (!eff_id.is_empty()) {
        return eff_id;
    }
    godot::String fallback = godot::String("Rule_") +
                             godot::String(target_class) + "_" +
                             godot::String(target_title_or_group) + "_" +
                             godot::String(target_property) + "_" +
                             godot::String::num_int64(static_cast<int64_t>(property_target));
    return godot::StringName(fallback);
}

void GameplayRule::_resolve_clamp_bounds(double &r_min, double &r_max) const {
    r_min = static_cast<double>(clamp_min);
    r_max = static_cast<double>(clamp_max);

    if (value.get_type() == godot::Variant::VECTOR2) {
        godot::Vector2 v = value;
        r_min = v.x;
        r_max = v.y;
    } else if (value.get_type() == godot::Variant::VECTOR2I) {
        godot::Vector2i v = value;
        r_min = static_cast<double>(v.x);
        r_max = static_cast<double>(v.y);
    } else if (value.get_type() == godot::Variant::ARRAY) {
        godot::Array arr = value;
        if (arr.size() >= 2) {
            r_min = static_cast<double>(arr[0]);
            r_max = static_cast<double>(arr[1]);
        }
    } else if (value.get_type() == godot::Variant::DICTIONARY) {
        godot::Dictionary dict = value;
        if (dict.has("min")) r_min = static_cast<double>(dict["min"]);
        if (dict.has("max")) r_max = static_cast<double>(dict["max"]);
    }

    if (r_min > r_max) {
        double tmp = r_min;
        r_min = r_max;
        r_max = tmp;
    }
}

// --- Matching & Application ---

GameProperty *GameplayRule::find_target_game_property_on_object(godot::Object *p_object) const {
    if (!p_object) {
        return nullptr;
    }

    if (GameProperty *direct_prop = godot::Object::cast_to<GameProperty>(p_object)) {
        if (target_property.is_empty()) {
            return direct_prop;
        }
        godot::StringName base_prop;
        RulePropertyTarget sub_target;
        _parse_target_property(base_prop, sub_target);
        godot::String base_str(base_prop);
        if (direct_prop->get_title() == base_str || direct_prop->get_name() == base_prop) {
            return direct_prop;
        }
    }

    if (target_property.is_empty()) {
        return nullptr;
    }

    godot::StringName base_prop;
    RulePropertyTarget sub_target;
    _parse_target_property(base_prop, sub_target);
    godot::String target_prop_str(base_prop);

    // 1. Fast path for GamePiece, which explicitly indexes its GameProperty array
    if (GamePiece *piece = godot::Object::cast_to<GamePiece>(p_object)) {
        if (GameProperty *prop = piece->get_property(target_prop_str)) {
            return prop;
        }
    }

    // 2. General path for any Node holding child GameProperty nodes
    if (godot::Node *node = godot::Object::cast_to<godot::Node>(p_object)) {
        int child_count = node->get_child_count();
        for (int i = 0; i < child_count; ++i) {
            if (GameProperty *prop = godot::Object::cast_to<GameProperty>(node->get_child(i))) {
                if (prop->get_title() == target_prop_str || prop->get_name() == base_prop) {
                    return prop;
                }
            }
        }
    }

    return nullptr;
}

GameProperty *GameplayRule::find_target_game_property(GameEntity *p_entity) const {
    return find_target_game_property_on_object(p_entity);
}

bool GameplayRule::matches_part_signature(const godot::Object *p_object) const {
    if (!p_object) {
        return false;
    }

    // 1. Check required_properties
    for (int i = 0; i < required_properties.size(); ++i) {
        godot::StringName req_prop = required_properties[i];
        if (req_prop.is_empty()) continue;

        bool found = _object_has_direct_property(p_object, req_prop);
        if (!found) {
            if (const GamePiece *piece = godot::Object::cast_to<GamePiece>(p_object)) {
                found = piece->has_property_name(godot::String(req_prop));
            } else if (const godot::Node *node = godot::Object::cast_to<godot::Node>(p_object)) {
                godot::String req_str(req_prop);
                int child_count = node->get_child_count();
                for (int c = 0; c < child_count; ++c) {
                    if (const GameProperty *gp = godot::Object::cast_to<GameProperty>(node->get_child(c))) {
                        if (gp->get_title() == req_str || gp->get_name() == req_prop) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!found) {
            return false;
        }
    }

    // 2. Check required_methods
    for (int i = 0; i < required_methods.size(); ++i) {
        godot::StringName req_method = required_methods[i];
        if (!req_method.is_empty() && !p_object->has_method(req_method)) {
            return false;
        }
    }

    // 3. Check required_signals
    for (int i = 0; i < required_signals.size(); ++i) {
        godot::StringName req_sig = required_signals[i];
        if (!req_sig.is_empty() && !p_object->has_signal(req_sig)) {
            return false;
        }
    }

    return true;
}

bool GameplayRule::matches_pseudo_state(godot::Object *p_object) const {
    if (!p_object) {
        return false;
    }

    // 1. Evaluate Narrative / Gameplay_Condition if assigned
    if (gameplay_condition.is_valid()) {
        NarremeConditionStatus status = gameplay_condition->evaluate_condition();
        if (status != NarremeConditionStatus::MET && status != NarremeConditionStatus::PERMANENT) {
            return false;
        }
    }

    // 2. Evaluate Pseudo-State (:state) selector if non-empty
    if (pseudo_state.is_empty()) {
        return true;
    }

    godot::String state_str(pseudo_state);
    state_str = state_str.strip_edges();
    bool negate = false;
    if (state_str.begins_with("!")) {
        negate = true;
        state_str = state_str.substr(1).strip_edges();
    }
    if (state_str.begins_with(":")) {
        state_str = state_str.substr(1);
    }
    if (state_str.begins_with("not_")) {
        negate = !negate;
        state_str = state_str.substr(4);
    }

    godot::String lower_state = state_str.to_lower();
    GameProperty *target_gp = find_target_game_property_on_object(p_object);
    bool matched = false;
    bool handled = false;

    // Built-in domain pseudo-states
    if (lower_state == "is_exhausted" || lower_state == "exhausted") {
        if (target_gp) {
            matched = target_gp->get_is_exhausted();
            handled = true;
        } else if (GameProperty *self_gp = godot::Object::cast_to<GameProperty>(p_object)) {
            matched = self_gp->get_is_exhausted();
            handled = true;
        }
    } else if (lower_state == "locked") {
        if (target_gp) {
            matched = target_gp->get_locked();
            handled = true;
        } else if (GameProperty *self_gp = godot::Object::cast_to<GameProperty>(p_object)) {
            matched = self_gp->get_locked();
            handled = true;
        }
    } else if (lower_state == "at_max" || lower_state == "full") {
        GameProperty *gp = target_gp ? target_gp : godot::Object::cast_to<GameProperty>(p_object);
        if (gp) {
            matched = gp->get_value() >= gp->get_modded_max_value();
            handled = true;
        }
    } else if (lower_state == "at_min" || lower_state == "empty") {
        GameProperty *gp = target_gp ? target_gp : godot::Object::cast_to<GameProperty>(p_object);
        if (gp) {
            matched = gp->get_value() <= gp->get_modded_min_value();
            handled = true;
        }
    } else if (lower_state == "in_progress") {
        if (GameAgentAction *act = godot::Object::cast_to<GameAgentAction>(p_object)) {
            matched = act->get_in_progress();
            handled = true;
        }
    } else if (lower_state == "enabled") {
        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(p_object)) {
            matched = ent->get_enabled();
            handled = true;
        }
    } else if (lower_state == "disabled") {
        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(p_object)) {
            matched = !ent->get_enabled();
            handled = true;
        }
    } else if (lower_state == "paused" || lower_state == "is_paused") {
        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(p_object)) {
            matched = ent->get_entity_is_paused();
            handled = true;
        } else if (Game *game = godot::Object::cast_to<Game>(p_object)) {
            matched = (game->get_game_state() == GameState::PAUSED);
            handled = true;
        }
    } else if (lower_state == "playing") {
        if (Game *game = godot::Object::cast_to<Game>(p_object)) {
            matched = (game->get_game_state() == GameState::PLAYING);
            handled = true;
        }
    } else if (lower_state == "possessed" || lower_state == "controlled") {
        if (GameAgent *agent = godot::Object::cast_to<GameAgent>(p_object)) {
            matched = (agent->get_player() != nullptr);
            handled = true;
        }
    } else if (lower_state == "unpossessed" || lower_state == "ai") {
        if (GameAgent *agent = godot::Object::cast_to<GameAgent>(p_object)) {
            matched = (agent->get_player() == nullptr);
            handled = true;
        }
    } else if (lower_state == "active" || lower_state == "interaction_active") {
        if (GameInteraction *inter = godot::Object::cast_to<GameInteraction>(p_object)) {
            matched = inter->is_interaction_active();
            handled = true;
        }
    }

    // Fallback: check boolean property or 0-arg getter method on p_object or target_gp
    if (!handled) {
        godot::StringName prop_name(state_str);
        if (_object_has_direct_property(p_object, prop_name)) {
            matched = static_cast<bool>(p_object->get(prop_name));
            handled = true;
        } else if (p_object->has_method(prop_name)) {
            matched = static_cast<bool>(const_cast<godot::Object *>(p_object)->call(prop_name));
            handled = true;
        } else if (target_gp && _object_has_direct_property(target_gp, prop_name)) {
            matched = static_cast<bool>(target_gp->get(prop_name));
            handled = true;
        }
    }

    if (!handled) {
        return false;
    }

    return negate ? !matched : matched;
}

bool GameplayRule::matches_object(godot::Object *p_object, bool p_is_self) const {
    if (!p_object || !get_enabled()) {
        return false;
    }

    // 1. Check Scope
    if (p_is_self && !applies_to_self()) {
        return false;
    }
    if (!p_is_self && !applies_to_descendants()) {
        return false;
    }

    // 2. Check Target Class (respects Godot ClassDB inheritance, e.g., "Game", "GameEntity", "GameAgent", "GamePiece")
    if (!target_class.is_empty()) {
        if (!p_object->is_class(godot::String(target_class))) {
            return false;
        }
    }

    // 3. Check Target Title or Group
    if (!target_title_or_group.is_empty()) {
        godot::String target_str(target_title_or_group);
        bool title_or_group_matched = false;

        if (GameEntity *ent = godot::Object::cast_to<GameEntity>(p_object)) {
            if (ent->get_title() == target_str) {
                title_or_group_matched = true;
            }
        } else if (Game *game = godot::Object::cast_to<Game>(p_object)) {
            if (game->get_title() == target_str) {
                title_or_group_matched = true;
            }
        } else if (GameProperty *gp = godot::Object::cast_to<GameProperty>(p_object)) {
            if (gp->get_title() == target_str) {
                title_or_group_matched = true;
            }
        } else if (_object_has_direct_property(p_object, godot::StringName("title"))) {
            if (godot::String(p_object->get(godot::StringName("title"))) == target_str) {
                title_or_group_matched = true;
            }
        }

        if (!title_or_group_matched) {
            if (godot::Node *node = godot::Object::cast_to<godot::Node>(p_object)) {
                if (node->get_name() == target_title_or_group || node->is_in_group(target_title_or_group)) {
                    title_or_group_matched = true;
                }
            }
        }

        if (!title_or_group_matched) {
            if (GameAgentAction *agent_action = godot::Object::cast_to<GameAgentAction>(p_object)) {
                if (agent_action->get_competes_with_groups().has(target_str)) {
                    title_or_group_matched = true;
                }
            }
        }

        if (!title_or_group_matched) {
            return false;
        }
    }

    // 4. Check Structural Reflection (PartSignature)
    if (!matches_part_signature(p_object)) {
        return false;
    }

    // 5. Check Pseudo-State (:state) & Gameplay_Condition
    if (!matches_pseudo_state(p_object)) {
        return false;
    }

    // 6. Check Target Property existence (for property-targeted operations)
    if (operation == RuleOp::SIGNAL_CONNECT) {
        return signal_connector.is_valid() && godot::Object::cast_to<godot::Node>(p_object) != nullptr;
    }
    if (operation == RuleOp::NODE_RETARGET) {
        return node_retargeter.is_valid() && godot::Object::cast_to<godot::Node>(p_object) != nullptr;
    }
    if (operation == RuleOp::INTERACTION_WEIGHT) {
        return p_object->is_class("GameInteraction") || !target_property.is_empty();
    }
    if (operation == RuleOp::GROUP_ADD || operation == RuleOp::GROUP_REMOVE) {
        return godot::Object::cast_to<GameAgentAction>(p_object) != nullptr ||
               godot::Object::cast_to<godot::Node>(p_object) != nullptr;
    }

    if (!target_property.is_empty()) {
        godot::StringName base_prop;
        RulePropertyTarget sub_target;
        _parse_target_property(base_prop, sub_target);

        bool has_game_prop = (find_target_game_property_on_object(p_object) != nullptr);
        if (!has_game_prop && !_object_has_direct_property(p_object, base_prop)) {
            return false;
        }
    }

    return true;
}

bool GameplayRule::matches_entity(GameEntity *p_entity, bool p_is_self) const {
    return matches_object(p_entity, p_is_self);
}

// --- Payload 1: GameProperty & GamePropertyModifier Application ---

bool GameplayRule::_apply_to_game_property(GameProperty *p_prop, RulePropertyTarget p_sub_target) const {
    if (!p_prop) {
        return false;
    }

    // Handle Modifier injection / update / removal on value, max_value, or min_value
    if (operation == RuleOp::MODIFIER_ADD) {
        if (value.get_type() == godot::Variant::OBJECT) {
            if (GamePropertyModifier *existing_mod = godot::Object::cast_to<GamePropertyModifier>(value.operator godot::Object *())) {
                if (p_sub_target == RulePropertyTarget::PROP_TARGET_MAX_VALUE) {
                    p_prop->add_max_modifier(existing_mod);
                } else if (p_sub_target == RulePropertyTarget::PROP_TARGET_MIN_VALUE) {
                    p_prop->add_min_modifier(existing_mod);
                } else {
                    p_prop->add_modifier(existing_mod);
                }
                return true;
            }
        }

        // Check if this rule already injected a modifier on p_prop; update in-place to avoid node leaks
        godot::String tag_key(_get_modifier_meta_tag());
        godot::Dictionary mod_map;
        if (p_prop->has_meta(META_ACTIVE_MODIFIERS)) {
            godot::Variant m = p_prop->get_meta(META_ACTIVE_MODIFIERS);
            if (m.get_type() == godot::Variant::DICTIONARY) {
                mod_map = m;
            }
        }

        GamePropertyModifier *mod = nullptr;
        if (mod_map.has(tag_key)) {
            mod = godot::Object::cast_to<GamePropertyModifier>(mod_map[tag_key]);
        }

        bool is_new = (mod == nullptr);
        if (is_new) {
            mod = memnew(GamePropertyModifier);
            mod->set_name(tag_key);
            p_prop->add_child(mod);
            mod_map[tag_key] = mod;
            p_prop->set_meta(META_ACTIVE_MODIFIERS, mod_map);
        }

        mod->set_type(modifier_type);
        mod->set_time_limit(modifier_time_limit);
        mod->set_value(static_cast<int>(value));

        if (p_sub_target == RulePropertyTarget::PROP_TARGET_MAX_VALUE) {
            p_prop->add_max_modifier(mod);
        } else if (p_sub_target == RulePropertyTarget::PROP_TARGET_MIN_VALUE) {
            p_prop->add_min_modifier(mod);
        } else {
            p_prop->add_modifier(mod);
        }
        return true;
    }

    if (operation == RuleOp::MODIFIER_REMOVE) {
        return _revert_from_game_property(p_prop, p_sub_target);
    }

    // Handle boolean flag sub-targets on GameProperty
    switch (p_sub_target) {
        case RulePropertyTarget::PROP_TARGET_LOCKED:
            cache_base_value(p_prop, godot::StringName("locked"), p_prop->get_locked());
            p_prop->set_locked(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_RESTRICT_MAX:
            cache_base_value(p_prop, godot::StringName("restrict_value_to_current_max_value"), p_prop->get_restrict_value_to_current_max_value());
            p_prop->set_restrict_value_to_current_max_value(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_RESTRICT_MIN:
            cache_base_value(p_prop, godot::StringName("restrict_value_to_current_min_value"), p_prop->get_restrict_value_to_current_min_value());
            p_prop->set_restrict_value_to_current_min_value(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_SET:
            cache_base_value(p_prop, godot::StringName("allow_value_set"), p_prop->get_allow_value_set());
            p_prop->set_allow_value_set(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_MODS:
            cache_base_value(p_prop, godot::StringName("allow_value_modifiers"), p_prop->get_allow_value_modifiers());
            p_prop->set_allow_value_modifiers(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MAX_SET:
            cache_base_value(p_prop, godot::StringName("allow_max_value_set"), p_prop->get_allow_max_value_set());
            p_prop->set_allow_max_value_set(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MAX_MODS:
            cache_base_value(p_prop, godot::StringName("allow_max_modifiers"), p_prop->get_allow_max_modifiers());
            p_prop->set_allow_max_modifiers(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MIN_SET:
            cache_base_value(p_prop, godot::StringName("allow_min_value_set"), p_prop->get_allow_min_value_set());
            p_prop->set_allow_min_value_set(static_cast<bool>(value));
            return true;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MIN_MODS:
            cache_base_value(p_prop, godot::StringName("allow_min_modifiers"), p_prop->get_allow_min_modifiers());
            p_prop->set_allow_min_modifiers(static_cast<bool>(value));
            return true;
        default:
            break;
    }

    // Numeric value / max_value / min_value operations with Base Value caching
    int current_val = 0;
    godot::StringName prop_key("value");
    if (p_sub_target == RulePropertyTarget::PROP_TARGET_MAX_VALUE) {
        current_val = p_prop->get_max_value();
        prop_key = godot::StringName("max_value");
    } else if (p_sub_target == RulePropertyTarget::PROP_TARGET_MIN_VALUE) {
        current_val = p_prop->get_min_value();
        prop_key = godot::StringName("min_value");
    } else {
        current_val = p_prop->get_value();
    }

    cache_base_value(p_prop, prop_key, current_val);

    int computed_val = current_val;
    switch (operation) {
        case RuleOp::SET:
            computed_val = static_cast<int>(value);
            break;
        case RuleOp::ADD:
            computed_val = current_val + static_cast<int>(value);
            break;
        case RuleOp::MULTIPLY:
            computed_val = static_cast<int>(godot::Math::round(static_cast<double>(current_val) * static_cast<double>(value)));
            break;
        case RuleOp::CLAMP: {
            double c_min = 0.0;
            double c_max = 1.0;
            _resolve_clamp_bounds(c_min, c_max);
            computed_val = godot::Math::clamp(current_val, static_cast<int>(c_min), static_cast<int>(c_max));
            break;
        }
        default:
            return false;
    }

    if (p_sub_target == RulePropertyTarget::PROP_TARGET_MAX_VALUE) {
        p_prop->set_max_value(computed_val);
        // If value was also at base max or needs scaling with max_value on initial SET
    } else if (p_sub_target == RulePropertyTarget::PROP_TARGET_MIN_VALUE) {
        p_prop->set_min_value(computed_val);
    } else {
        // If scaling/setting `value` above current `max_value`, also expand `max_value` so `set_value` isn't clamped away
        if (computed_val > p_prop->get_modded_max_value() && p_prop->get_allow_max_value_set()) {
            cache_base_value(p_prop, godot::StringName("max_value"), p_prop->get_max_value());
            p_prop->set_max_value(computed_val);
        }
        p_prop->set_value(computed_val);
    }

    return true;
}

bool GameplayRule::_revert_from_game_property(GameProperty *p_prop, RulePropertyTarget p_sub_target) const {
    if (!p_prop) {
        return false;
    }

    bool changed = false;

    // 1. Remove any modifier injected by this rule
    if (p_prop->has_meta(META_ACTIVE_MODIFIERS)) {
        godot::String tag_key(_get_modifier_meta_tag());
        godot::Dictionary mod_map = p_prop->get_meta(META_ACTIVE_MODIFIERS);
        if (mod_map.has(tag_key)) {
            if (GamePropertyModifier *mod = godot::Object::cast_to<GamePropertyModifier>(mod_map[tag_key])) {
                p_prop->remove_modifier(mod);
                p_prop->remove_max_modifier(mod);
                p_prop->remove_min_modifier(mod);
                if (mod->get_parent() == p_prop) {
                    p_prop->remove_child(mod);
                    mod->queue_free();
                }
                changed = true;
            }
            mod_map.erase(tag_key);
            p_prop->set_meta(META_ACTIVE_MODIFIERS, mod_map);
        }
    }

    // 2. If value is a direct GamePropertyModifier reference on MODIFIER_REMOVE, remove it too
    if (operation == RuleOp::MODIFIER_REMOVE && value.get_type() == godot::Variant::OBJECT) {
        if (GamePropertyModifier *mod = godot::Object::cast_to<GamePropertyModifier>(value.operator godot::Object *())) {
            p_prop->remove_modifier(mod);
            p_prop->remove_max_modifier(mod);
            p_prop->remove_min_modifier(mod);
            changed = true;
        }
    }

    // 3. Restore cached base value for the targeted sub-property
    godot::StringName prop_key("value");
    switch (p_sub_target) {
        case RulePropertyTarget::PROP_TARGET_MAX_VALUE: prop_key = godot::StringName("max_value"); break;
        case RulePropertyTarget::PROP_TARGET_MIN_VALUE: prop_key = godot::StringName("min_value"); break;
        case RulePropertyTarget::PROP_TARGET_LOCKED: prop_key = godot::StringName("locked"); break;
        case RulePropertyTarget::PROP_TARGET_RESTRICT_MAX: prop_key = godot::StringName("restrict_value_to_current_max_value"); break;
        case RulePropertyTarget::PROP_TARGET_RESTRICT_MIN: prop_key = godot::StringName("restrict_value_to_current_min_value"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_SET: prop_key = godot::StringName("allow_value_set"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_VALUE_MODS: prop_key = godot::StringName("allow_value_modifiers"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MAX_SET: prop_key = godot::StringName("allow_max_value_set"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MAX_MODS: prop_key = godot::StringName("allow_max_modifiers"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MIN_SET: prop_key = godot::StringName("allow_min_value_set"); break;
        case RulePropertyTarget::PROP_TARGET_ALLOW_MIN_MODS: prop_key = godot::StringName("allow_min_modifiers"); break;
        default: break;
    }

    if (restore_base_value(p_prop, prop_key)) {
        changed = true;
    }

    return changed;
}

// --- Payload 3: Action & Interaction Laws ---

bool GameplayRule::_apply_group_operation(godot::Object *p_object, bool p_add) const {
    if (!p_object) {
        return false;
    }

    godot::TypedArray<godot::String> tags_to_apply;
    if (value.get_type() == godot::Variant::STRING || value.get_type() == godot::Variant::STRING_NAME) {
        godot::String s(value);
        if (!s.is_empty()) {
            tags_to_apply.append(s);
        }
    } else if (value.get_type() == godot::Variant::ARRAY) {
        godot::Array arr = value;
        for (int i = 0; i < arr.size(); ++i) {
            godot::String s(arr[i]);
            if (!s.is_empty()) {
                tags_to_apply.append(s);
            }
        }
    }

    if (tags_to_apply.is_empty()) {
        return false;
    }

    // 1. If targeting GameAgentAction (and target_property is empty or "competes_with_groups"), modify competes_with_groups
    if (GameAgentAction *action = godot::Object::cast_to<GameAgentAction>(p_object)) {
        if (target_property.is_empty() || target_property == godot::StringName("competes_with_groups")) {
            cache_base_value(action, godot::StringName("competes_with_groups"), action->get_competes_with_groups().duplicate());
            godot::TypedArray<godot::String> groups = action->get_competes_with_groups().duplicate();
            for (int i = 0; i < tags_to_apply.size(); ++i) {
                godot::String tag = tags_to_apply[i];
                if (p_add) {
                    if (!groups.has(tag)) {
                        groups.append(tag);
                    }
                } else {
                    int idx = groups.find(tag);
                    if (idx >= 0) {
                        groups.remove_at(idx);
                    }
                }
            }
            action->set_competes_with_groups(groups);
            return true;
        }
    }

    // 2. Otherwise modify Godot Node groups on the target Node
    if (godot::Node *node = godot::Object::cast_to<godot::Node>(p_object)) {
        for (int i = 0; i < tags_to_apply.size(); ++i) {
            godot::String tag_str = tags_to_apply[i];
            godot::StringName grp(tag_str);
            if (p_add) {
                if (!node->is_in_group(grp)) {
                    node->add_to_group(grp);
                }
            } else {
                if (node->is_in_group(grp)) {
                    node->remove_from_group(grp);
                }
            }
        }
        return true;
    }

    return false;
}

bool GameplayRule::_apply_interaction_weights(godot::Object *p_object) const {
    if (!p_object) {
        return false;
    }

    p_object->set_meta(META_INTERACTION_INSTIGATOR_WEIGHT, instigator_weight);
    p_object->set_meta(META_INTERACTION_COMPETITION_WEIGHT, competition_weight);
    p_object->set_meta(META_INTERACTION_CONSEQUENCE_MULT, consequence_score_multiplier);

    if (_object_has_direct_property(p_object, godot::StringName("instigator_weight"))) {
        cache_base_value(p_object, godot::StringName("instigator_weight"), p_object->get(godot::StringName("instigator_weight")));
        p_object->set(godot::StringName("instigator_weight"), instigator_weight);
    }
    if (_object_has_direct_property(p_object, godot::StringName("competition_weight"))) {
        cache_base_value(p_object, godot::StringName("competition_weight"), p_object->get(godot::StringName("competition_weight")));
        p_object->set(godot::StringName("competition_weight"), competition_weight);
    }
    if (_object_has_direct_property(p_object, godot::StringName("consequence_score_multiplier"))) {
        cache_base_value(p_object, godot::StringName("consequence_score_multiplier"), p_object->get(godot::StringName("consequence_score_multiplier")));
        p_object->set(godot::StringName("consequence_score_multiplier"), consequence_score_multiplier);
    }

    return true;
}

// --- Payload 4: Reactive Signal & Node Retarget Hooks ---

bool GameplayRule::_apply_reactive_hooks(godot::Object *p_object) const {
    godot::Node *node = godot::Object::cast_to<godot::Node>(p_object);
    if (!node) {
        return false;
    }

    godot::Node *root = node;
    if (GameEntity *ent = godot::Object::cast_to<GameEntity>(node)) {
        if (ent->get_root_node()) {
            root = ent->get_root_node();
        }
    }

    bool applied = false;

    if ((operation == RuleOp::SIGNAL_CONNECT || signal_connector.is_valid()) && signal_connector.is_valid()) {
        signal_connector->connect_signals(root, root);
        godot::Array active_list;
        if (node->has_meta(META_ACTIVE_SIGNAL_CONNECTORS)) {
            active_list = node->get_meta(META_ACTIVE_SIGNAL_CONNECTORS);
        }
        if (!active_list.has(signal_connector)) {
            active_list.append(signal_connector);
            node->set_meta(META_ACTIVE_SIGNAL_CONNECTORS, active_list);
        }
        applied = true;
    }

    if ((operation == RuleOp::NODE_RETARGET || node_retargeter.is_valid()) && node_retargeter.is_valid()) {
        node_retargeter->retarget(root, root);
        godot::Array active_list;
        if (node->has_meta(META_ACTIVE_NODE_RETARGETERS)) {
            active_list = node->get_meta(META_ACTIVE_NODE_RETARGETERS);
        }
        if (!active_list.has(node_retargeter)) {
            active_list.append(node_retargeter);
            node->set_meta(META_ACTIVE_NODE_RETARGETERS, active_list);
        }
        applied = true;
    }

    return applied;
}

bool GameplayRule::_revert_reactive_hooks(godot::Object *p_object) const {
    godot::Node *node = godot::Object::cast_to<godot::Node>(p_object);
    if (!node) {
        return false;
    }

    godot::Node *root = node;
    if (GameEntity *ent = godot::Object::cast_to<GameEntity>(node)) {
        if (ent->get_root_node()) {
            root = ent->get_root_node();
        }
    }

    bool reverted = false;

    if (signal_connector.is_valid()) {
        signal_connector->disconnect_signals(root, root);
        if (node->has_meta(META_ACTIVE_SIGNAL_CONNECTORS)) {
            godot::Array active_list = node->get_meta(META_ACTIVE_SIGNAL_CONNECTORS);
            int idx = active_list.find(signal_connector);
            if (idx >= 0) {
                active_list.remove_at(idx);
                node->set_meta(META_ACTIVE_SIGNAL_CONNECTORS, active_list);
            }
        }
        reverted = true;
    }

    if (node_retargeter.is_valid()) {
        node_retargeter->clear_set_targets(root, root);
        if (node->has_meta(META_ACTIVE_NODE_RETARGETERS)) {
            godot::Array active_list = node->get_meta(META_ACTIVE_NODE_RETARGETERS);
            int idx = active_list.find(node_retargeter);
            if (idx >= 0) {
                active_list.remove_at(idx);
                node->set_meta(META_ACTIVE_NODE_RETARGETERS, active_list);
            }
        }
        reverted = true;
    }

    return reverted;
}

// --- Public Application & Rollback ---

bool GameplayRule::apply_to_object(godot::Object *p_object, bool p_is_self) const {
    if (!matches_object(p_object, p_is_self)) {
        return false;
    }

    // Special operations that do not require a target_property
    if (operation == RuleOp::SIGNAL_CONNECT || operation == RuleOp::NODE_RETARGET) {
        return _apply_reactive_hooks(p_object);
    }
    if (operation == RuleOp::GROUP_ADD) {
        return _apply_group_operation(p_object, true);
    }
    if (operation == RuleOp::GROUP_REMOVE) {
        return _apply_group_operation(p_object, false);
    }
    if (operation == RuleOp::INTERACTION_WEIGHT) {
        return _apply_interaction_weights(p_object);
    }

    // Also attach any auxiliary SignalConnector / NodeRetargeter configured alongside a property rule
    if (signal_connector.is_valid() || node_retargeter.is_valid()) {
        _apply_reactive_hooks(p_object);
    }

    if (target_property.is_empty()) {
        if (GameProperty *direct_prop = godot::Object::cast_to<GameProperty>(p_object)) {
            return _apply_to_game_property(direct_prop, property_target);
        }
        return signal_connector.is_valid() || node_retargeter.is_valid();
    }

    godot::StringName base_prop;
    RulePropertyTarget sub_target;
    _parse_target_property(base_prop, sub_target);

    // Case 1: Target resolves to a child (or self) GameProperty (e.g., "Health", "Health:max_value", "Stamina:locked")
    if (GameProperty *game_prop = find_target_game_property_on_object(p_object)) {
        return _apply_to_game_property(game_prop, sub_target);
    }

    // Case 2: Target is a direct property on the Object (Game, GameEntity, GameAgentAction, etc.)
    if (!_object_has_direct_property(p_object, base_prop)) {
        return false;
    }

    // Special case: ADD / GROUP_ADD on `competes_with_groups` Array property
    if (base_prop == godot::StringName("competes_with_groups") && operation == RuleOp::ADD) {
        return _apply_group_operation(p_object, true);
    }

    godot::Variant current = p_object->get(base_prop);
    cache_base_value(p_object, base_prop, current);

    switch (operation) {
        case RuleOp::SET: {
            p_object->set(base_prop, value);
            return true;
        }
        case RuleOp::ADD: {
            if (current.get_type() == godot::Variant::FLOAT || value.get_type() == godot::Variant::FLOAT) {
                p_object->set(base_prop, static_cast<double>(current) + static_cast<double>(value));
                return true;
            } else if (current.get_type() == godot::Variant::INT) {
                p_object->set(base_prop, static_cast<int64_t>(current) + static_cast<int64_t>(value));
                return true;
            } else if (current.get_type() == godot::Variant::STRING) {
                p_object->set(base_prop, godot::String(current) + godot::String(value));
                return true;
            }
            return false;
        }
        case RuleOp::MULTIPLY: {
            if (current.get_type() == godot::Variant::FLOAT || value.get_type() == godot::Variant::FLOAT) {
                p_object->set(base_prop, static_cast<double>(current) * static_cast<double>(value));
                return true;
            } else if (current.get_type() == godot::Variant::INT) {
                p_object->set(base_prop, static_cast<int64_t>(godot::Math::round(static_cast<double>(current) * static_cast<double>(value))));
                return true;
            }
            return false;
        }
        case RuleOp::CLAMP: {
            double c_min = 0.0;
            double c_max = 1.0;
            _resolve_clamp_bounds(c_min, c_max);
            if (current.get_type() == godot::Variant::FLOAT) {
                p_object->set(base_prop, godot::Math::clamp(static_cast<double>(current), c_min, c_max));
                return true;
            } else if (current.get_type() == godot::Variant::INT) {
                p_object->set(base_prop, godot::Math::clamp(static_cast<int64_t>(current), static_cast<int64_t>(c_min), static_cast<int64_t>(c_max)));
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

bool GameplayRule::apply_to_entity(GameEntity *p_entity, bool p_is_self) const {
    return apply_to_object(p_entity, p_is_self);
}

bool GameplayRule::revert_from_object(godot::Object *p_object) const {
    if (!p_object) {
        return false;
    }

    bool reverted = false;

    if (signal_connector.is_valid() || node_retargeter.is_valid() ||
        operation == RuleOp::SIGNAL_CONNECT || operation == RuleOp::NODE_RETARGET) {
        if (_revert_reactive_hooks(p_object)) {
            reverted = true;
        }
    }

    if (operation == RuleOp::INTERACTION_WEIGHT) {
        if (p_object->has_meta(META_INTERACTION_INSTIGATOR_WEIGHT)) {
            p_object->remove_meta(META_INTERACTION_INSTIGATOR_WEIGHT);
            reverted = true;
        }
        if (p_object->has_meta(META_INTERACTION_COMPETITION_WEIGHT)) {
            p_object->remove_meta(META_INTERACTION_COMPETITION_WEIGHT);
            reverted = true;
        }
        if (p_object->has_meta(META_INTERACTION_CONSEQUENCE_MULT)) {
            p_object->remove_meta(META_INTERACTION_CONSEQUENCE_MULT);
            reverted = true;
        }
        restore_base_value(p_object, godot::StringName("instigator_weight"));
        restore_base_value(p_object, godot::StringName("competition_weight"));
        restore_base_value(p_object, godot::StringName("consequence_score_multiplier"));
    }

    if (operation == RuleOp::GROUP_ADD || operation == RuleOp::GROUP_REMOVE) {
        if (restore_base_value(p_object, godot::StringName("competes_with_groups"))) {
            reverted = true;
        }
    }

    if (!target_property.is_empty()) {
        godot::StringName base_prop;
        RulePropertyTarget sub_target;
        _parse_target_property(base_prop, sub_target);

        if (GameProperty *game_prop = find_target_game_property_on_object(p_object)) {
            if (_revert_from_game_property(game_prop, sub_target)) {
                reverted = true;
            }
        } else if (restore_base_value(p_object, base_prop)) {
            reverted = true;
        }
    } else if (GameProperty *direct_prop = godot::Object::cast_to<GameProperty>(p_object)) {
        if (_revert_from_game_property(direct_prop, property_target)) {
            reverted = true;
        }
    }

    return reverted;
}

bool GameplayRule::revert_from_entity(GameEntity *p_entity) const {
    return revert_from_object(p_entity);
}

// --- Persistence & GameplayResource Overrides ---

void GameplayRule::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    GameplayResource::apply_resource(p_other);
    godot::Ref<GameplayRule> other_rule = p_other;
    if (!other_rule.is_valid()) {
        return;
    }

    scope = other_rule->get_scope();
    cascade_layer = other_rule->get_cascade_layer();
    target_class = other_rule->get_target_class();
    target_title_or_group = other_rule->get_target_title_or_group();
    target_property = other_rule->get_target_property();
    required_properties = other_rule->get_required_properties().duplicate();
    required_methods = other_rule->get_required_methods().duplicate();
    required_signals = other_rule->get_required_signals().duplicate();
    pseudo_state = other_rule->get_pseudo_state();
    gameplay_condition = other_rule->get_gameplay_condition();

    operation = other_rule->get_operation();
    value = other_rule->get_value();
    clamp_min = other_rule->get_clamp_min();
    clamp_max = other_rule->get_clamp_max();
    priority = other_rule->get_priority();
    is_important = other_rule->get_is_important();

    property_target = other_rule->get_property_target();
    modifier_type = other_rule->get_modifier_type();
    modifier_time_limit = other_rule->get_modifier_time_limit();

    instigator_weight = other_rule->get_instigator_weight();
    competition_weight = other_rule->get_competition_weight();
    consequence_score_multiplier = other_rule->get_consequence_score_multiplier();

    signal_connector = other_rule->get_signal_connector();
    node_retargeter = other_rule->get_node_retargeter();

    emit_changed();
}

godot::Dictionary GameplayRule::save_resource() const {
    return save_rule();
}

void GameplayRule::load_resource(const godot::Dictionary &p_data) {
    load_rule(p_data);
}

godot::Dictionary GameplayRule::save_rule() const {
    godot::Dictionary data = GameplayResource::save_resource();
    data["scope"] = static_cast<int>(scope);
    data["cascade_layer"] = static_cast<int>(cascade_layer);
    data["target_class"] = godot::String(target_class);
    data["target_title_or_group"] = godot::String(target_title_or_group);
    data["target_property"] = godot::String(target_property);

    godot::Array req_props;
    for (int i = 0; i < required_properties.size(); ++i) {
        req_props.append(godot::String(godot::StringName(required_properties[i])));
    }
    data["required_properties"] = req_props;

    godot::Array req_methods;
    for (int i = 0; i < required_methods.size(); ++i) {
        req_methods.append(godot::String(godot::StringName(required_methods[i])));
    }
    data["required_methods"] = req_methods;

    godot::Array req_sigs;
    for (int i = 0; i < required_signals.size(); ++i) {
        req_sigs.append(godot::String(godot::StringName(required_signals[i])));
    }
    data["required_signals"] = req_sigs;

    data["pseudo_state"] = godot::String(pseudo_state);

    data["operation"] = static_cast<int>(operation);
    data["value"] = value;
    data["clamp_min"] = clamp_min;
    data["clamp_max"] = clamp_max;
    data["priority"] = priority;
    data["is_important"] = is_important;

    data["property_target"] = static_cast<int>(property_target);
    data["modifier_type"] = static_cast<int>(modifier_type);
    data["modifier_time_limit"] = modifier_time_limit;

    data["instigator_weight"] = instigator_weight;
    data["competition_weight"] = competition_weight;
    data["consequence_score_multiplier"] = consequence_score_multiplier;

    return data;
}

void GameplayRule::load_rule(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("scope")) {
        scope = static_cast<RuleScope>(static_cast<int>(p_data["scope"]));
    }
    if (p_data.has("cascade_layer")) {
        cascade_layer = static_cast<RuleCascadeLayer>(static_cast<int>(p_data["cascade_layer"]));
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

    if (p_data.has("required_properties") && p_data["required_properties"].get_type() == godot::Variant::ARRAY) {
        required_properties.clear();
        godot::Array arr = p_data["required_properties"];
        for (int i = 0; i < arr.size(); ++i) {
            required_properties.append(godot::StringName(godot::String(arr[i])));
        }
    }
    if (p_data.has("required_methods") && p_data["required_methods"].get_type() == godot::Variant::ARRAY) {
        required_methods.clear();
        godot::Array arr = p_data["required_methods"];
        for (int i = 0; i < arr.size(); ++i) {
            required_methods.append(godot::StringName(godot::String(arr[i])));
        }
    }
    if (p_data.has("required_signals") && p_data["required_signals"].get_type() == godot::Variant::ARRAY) {
        required_signals.clear();
        godot::Array arr = p_data["required_signals"];
        for (int i = 0; i < arr.size(); ++i) {
            required_signals.append(godot::StringName(godot::String(arr[i])));
        }
    }

    if (p_data.has("pseudo_state")) {
        pseudo_state = godot::StringName(godot::String(p_data["pseudo_state"]));
    }

    if (p_data.has("operation")) {
        operation = static_cast<RuleOp>(static_cast<int>(p_data["operation"]));
    }
    if (p_data.has("value")) {
        value = p_data["value"];
    }
    if (p_data.has("clamp_min")) {
        clamp_min = p_data["clamp_min"];
    }
    if (p_data.has("clamp_max")) {
        clamp_max = p_data["clamp_max"];
    }
    if (p_data.has("priority")) {
        priority = p_data["priority"];
    }
    if (p_data.has("is_important")) {
        is_important = p_data["is_important"];
    }

    if (p_data.has("property_target")) {
        property_target = static_cast<RulePropertyTarget>(static_cast<int>(p_data["property_target"]));
    }
    if (p_data.has("modifier_type")) {
        modifier_type = static_cast<ModifierType>(static_cast<int>(p_data["modifier_type"]));
    }
    if (p_data.has("modifier_time_limit")) {
        modifier_time_limit = p_data["modifier_time_limit"];
    }

    if (p_data.has("instigator_weight")) {
        instigator_weight = p_data["instigator_weight"];
    }
    if (p_data.has("competition_weight")) {
        competition_weight = p_data["competition_weight"];
    }
    if (p_data.has("consequence_score_multiplier")) {
        consequence_score_multiplier = p_data["consequence_score_multiplier"];
    }

    emit_changed();
}

} // namespace ideam::godot_ext