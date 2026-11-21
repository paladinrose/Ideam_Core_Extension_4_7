#include "character.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "location.h"
#include "prop.h"
#include "incident.h"
#include "plot.h"
#include "narrative.h"
#include "../helpers/relationship.h"

#include <algorithm>

namespace ideam::godot_ext {

void Character::_bind_methods() {
    using namespace godot;

    // Properties
    ClassDB::bind_method(D_METHOD("set_current_location", "current_location"), &Character::set_current_location);
    ClassDB::bind_method(D_METHOD("get_current_location"), &Character::get_current_location);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "current_location", PROPERTY_HINT_RESOURCE_TYPE, "Location"), "set_current_location", "get_current_location");

    ClassDB::bind_method(D_METHOD("set_possessions", "possessions"), &Character::set_possessions);
    ClassDB::bind_method(D_METHOD("get_possessions"), &Character::get_possessions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "possessions", PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_possessions", "get_possessions");

    ClassDB::bind_method(D_METHOD("set_relationships", "relationships"), &Character::set_relationships);
    ClassDB::bind_method(D_METHOD("get_relationships"), &Character::get_relationships);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "relationships", PROPERTY_HINT_ARRAY_TYPE, "Relationship"), "set_relationships", "get_relationships");

    ClassDB::bind_method(D_METHOD("set_goals", "goals"), &Character::set_goals);
    ClassDB::bind_method(D_METHOD("get_goals"), &Character::get_goals);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "goals", PROPERTY_HINT_ARRAY_TYPE, "Plot"), "set_goals", "get_goals");

    // Location Methods
    ClassDB::bind_method(D_METHOD("enter_location", "newLocation"), &Character::enter_location);
    ClassDB::bind_method(D_METHOD("leave_current_location"), &Character::leave_current_location);

    // Possession Methods
    ClassDB::bind_method(D_METHOD("take_possession", "possession"), &Character::take_possession);
    ClassDB::bind_method(D_METHOD("get_possession_id", "possession"), &Character::get_possession_id);
    ClassDB::bind_method(D_METHOD("has_possession_of", "possession"), &Character::has_possession_of);
    ClassDB::bind_method(D_METHOD("lose_possession", "possession"), &Character::lose_possession);
    ClassDB::bind_method(D_METHOD("lose_possession_at", "id"), &Character::lose_possession_at);

    // Goal Methods
    ClassDB::bind_method(D_METHOD("set_goal", "goal"), &Character::set_goal);
    ClassDB::bind_method(D_METHOD("get_goal_id", "goal"), &Character::get_goal_id);
    ClassDB::bind_method(D_METHOD("has_goal", "goal"), &Character::has_goal);
    ClassDB::bind_method(D_METHOD("remove_goal", "goal"), &Character::remove_goal);
    ClassDB::bind_method(D_METHOD("remove_goal_at", "goal_ID"), &Character::remove_goal_at);
    ClassDB::bind_method(D_METHOD("check_goal_status", "goal"), &Character::check_goal_status);
    ClassDB::bind_method(D_METHOD("check_goal_status_at", "goal_ID"), &Character::check_goal_status_at);
    ClassDB::bind_method(D_METHOD("goal_success", "goal_ID"), &Character::goal_success);
    ClassDB::bind_method(D_METHOD("goal_fail", "goal_ID"), &Character::goal_fail);

    // Relationship Methods
    ClassDB::bind_method(D_METHOD("begin_relationship", "relation", "title"), &Character::begin_relationship);
    ClassDB::bind_method(D_METHOD("end_relationship", "relationship"), &Character::end_relationship);
    ClassDB::bind_method(D_METHOD("end_relationship_at", "id"), &Character::end_relationship_at);
    ClassDB::bind_method(D_METHOD("change_relation", "id", "newRelation"), &Character::change_relation);
    ClassDB::bind_method(D_METHOD("change_relationship", "id", "newTitle"), &Character::change_relationship);
    ClassDB::bind_method(D_METHOD("get_relationship_id", "title"), &Character::get_relationship_id);
    ClassDB::bind_method(D_METHOD("get_relation_ids", "target"), &Character::get_relation_ids);

    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Character::get_class_name_str);

    // Signals
    ADD_SIGNAL(MethodInfo("location_entered", PropertyInfo(Variant::OBJECT, "new_location", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(MethodInfo("location_exited", PropertyInfo(Variant::OBJECT, "location_exited", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(MethodInfo("possession_taken", PropertyInfo(Variant::OBJECT, "new_possession", PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(MethodInfo("possession_lost", PropertyInfo(Variant::OBJECT, "lost_possession", PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(MethodInfo("already_has_possession", PropertyInfo(Variant::OBJECT, "possession", PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(MethodInfo("relationship_started", PropertyInfo(Variant::OBJECT, "new_relationship", PROPERTY_HINT_RESOURCE_TYPE, "Relationship")));
    ADD_SIGNAL(MethodInfo("relationship_ended", PropertyInfo(Variant::OBJECT, "ended_relationship", PROPERTY_HINT_RESOURCE_TYPE, "Relationship")));
    ADD_SIGNAL(MethodInfo("relation_changed", PropertyInfo(Variant::OBJECT, "relationship", PROPERTY_HINT_RESOURCE_TYPE, "Relationship"), PropertyInfo(Variant::OBJECT, "old_relation", PROPERTY_HINT_RESOURCE_TYPE, "Narreme")));
    ADD_SIGNAL(MethodInfo("relationship_changed", PropertyInfo(Variant::OBJECT, "relationship", PROPERTY_HINT_RESOURCE_TYPE, "Relationship"), PropertyInfo(Variant::STRING, "old_title")));
    ADD_SIGNAL(MethodInfo("new_goal_set", PropertyInfo(Variant::OBJECT, "new_goal", PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(MethodInfo("goal_achieved", PropertyInfo(Variant::OBJECT, "achieved_goal", PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(MethodInfo("goal_failed", PropertyInfo(Variant::OBJECT, "failed_goal", PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(MethodInfo("goal_removed", PropertyInfo(Variant::OBJECT, "goal", PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
}

Character::Character() = default;
Character::~Character() = default;

void Character::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Character::set_current_location(const godot::Ref<Location> &p_location) {
    current_location = p_location;
    emit_changed();
}
godot::Ref<Location> Character::get_current_location() const { return current_location; }

void Character::set_possessions(const godot::TypedArray<Prop> &p_possessions) {
    possessions = p_possessions;
    emit_changed();
}
godot::TypedArray<Prop> Character::get_possessions() const { return possessions; }

void Character::set_relationships(const godot::TypedArray<Relationship> &p_relationships) {
    relationships = p_relationships;
    emit_changed();
}
godot::TypedArray<Relationship> Character::get_relationships() const { return relationships; }

void Character::set_goals(const godot::TypedArray<Plot> &p_goals) {
    goals = p_goals;
    emit_changed();
}
godot::TypedArray<Plot> Character::get_goals() const { return goals; }

// ----------------------------------------------------------------------------
// DIRECT INTERNAL MUTATORS
// ----------------------------------------------------------------------------

void Character::_apply_location_direct(const godot::Ref<Location> &p_new_location) {
    if (p_new_location == current_location) {
        return;
    }
    if (current_location.is_valid()) {
        godot::Ref<Location> exited = current_location;
        current_location.unref();
        emit_signal("location_exited", exited);
    }
    if (p_new_location.is_valid()) {
        current_location = p_new_location;
        emit_signal("location_entered", current_location);
    }
    emit_changed();
}

void Character::_insert_possession_at_direct(int p_index, const godot::Ref<Prop> &p_prop) {
    if (!p_prop.is_valid() || possessions.has(p_prop)) {
        return;
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(possessions.size()));
    possessions.insert(safe_idx, p_prop);
    emit_signal("possession_taken", p_prop);
    emit_changed();
}

void Character::_remove_possession_at_direct(int p_index) {
    if (p_index < 0 || p_index >= possessions.size()) {
        return;
    }
    godot::Ref<Prop> lost = possessions[p_index];
    possessions.remove_at(p_index);
    emit_signal("possession_lost", lost);
    emit_changed();
}

void Character::_insert_goal_at_direct(int p_index, const godot::Ref<Plot> &p_goal) {
    if (!p_goal.is_valid() || goals.has(p_goal)) {
        return;
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(goals.size()));
    goals.insert(safe_idx, p_goal);
    emit_signal("new_goal_set", p_goal);
    emit_changed();
}

void Character::_remove_goal_at_direct(int p_index) {
    if (p_index < 0 || p_index >= goals.size()) {
        return;
    }
    godot::Ref<Plot> removed = goals[p_index];
    goals.remove_at(p_index);
    emit_signal("goal_removed", removed);
    emit_changed();
}

void Character::_insert_relationship_at_direct(int p_index, const godot::Ref<Relationship> &p_relationship) {
    if (!p_relationship.is_valid()) {
        return;
    }
    if (undo_redo_.is_valid()) {
        p_relationship->set_undo_redo(undo_redo_);
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(relationships.size()));
    relationships.insert(safe_idx, p_relationship);
    emit_signal("relationship_started", p_relationship);
    emit_changed();
}

void Character::_remove_relationship_at_direct(int p_index) {
    if (p_index < 0 || p_index >= relationships.size()) {
        return;
    }
    godot::Ref<Relationship> ended = relationships[p_index];
    relationships.remove_at(p_index);
    emit_signal("relationship_ended", ended);
    emit_changed();
}

void Character::_set_relationship_relation_direct(int p_id, const godot::Ref<Narreme> &p_new_relation) {
    if (p_id < 0 || p_id >= relationships.size()) {
        return;
    }
    godot::Ref<Relationship> rel = relationships[p_id];
    if (!rel.is_valid()) {
        return;
    }
    godot::Ref<Narreme> old_relation = rel->get_relation();
    if (old_relation == p_new_relation) {
        return;
    }
    rel->set_relation(p_new_relation);
    emit_signal("relation_changed", rel, old_relation);
    emit_changed();
}

void Character::_set_relationship_title_direct(int p_id, const godot::String &p_new_title) {
    if (p_id < 0 || p_id >= relationships.size()) {
        return;
    }
    godot::Ref<Relationship> rel = relationships[p_id];
    if (!rel.is_valid()) {
        return;
    }
    const godot::String old_title = rel->get_title();
    if (old_title == p_new_title) {
        return;
    }
    rel->set_title(p_new_title);
    emit_signal("relationship_changed", rel, old_title);
    emit_changed();
}

// ----------------------------------------------------------------------------
// LOCATION MANAGEMENT
// ----------------------------------------------------------------------------

void Character::enter_location(const godot::Ref<Location> &p_new_location) {
    if (p_new_location == current_location) {
        return;
    }
    if (_can_record_undo()) {
        const godot::Ref<Location> old_location = current_location;
        undo_redo_->create_action("Character Enter Location");
        undo_redo_->add_do_method(callable_mp(this, &Character::_apply_location_direct).bind(p_new_location));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_apply_location_direct).bind(old_location));
        undo_redo_->commit_action();
    } else {
        _apply_location_direct(p_new_location);
    }
}

void Character::leave_current_location() {
    if (!current_location.is_valid()) {
        return;
    }
    if (_can_record_undo()) {
        const godot::Ref<Location> old_location = current_location;
        undo_redo_->create_action("Character Leave Location");
        undo_redo_->add_do_method(callable_mp(this, &Character::_apply_location_direct).bind(godot::Ref<Location>()));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_apply_location_direct).bind(old_location));
        undo_redo_->commit_action();
    } else {
        _apply_location_direct(godot::Ref<Location>());
    }
}

// ----------------------------------------------------------------------------
// POSSESSION MANAGEMENT
// ----------------------------------------------------------------------------

int Character::take_possession(const godot::Ref<Prop> &p_possession) {
    if (!p_possession.is_valid()) {
        return -1;
    }
    int id = get_possession_id(p_possession);
    if (id >= 0) {
        emit_signal("already_has_possession", p_possession);
    } else {
        const int target_idx = static_cast<int>(possessions.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Character Take Possession");
            undo_redo_->add_do_method(callable_mp(this, &Character::_insert_possession_at_direct).bind(target_idx, p_possession));
            undo_redo_->add_undo_method(callable_mp(this, &Character::_remove_possession_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_possession_at_direct(target_idx, p_possession);
        }
        id = get_possession_id(p_possession);
    }
    return id;
}

int Character::get_possession_id(const godot::Ref<Prop> &p_possession) const {
    return possessions.find(p_possession);
}

bool Character::has_possession_of(const godot::Ref<Prop> &p_possession) const {
    return possessions.has(p_possession);
}

void Character::lose_possession(const godot::Ref<Prop> &p_possession) {
    lose_possession_at(get_possession_id(p_possession));
}

void Character::lose_possession_at(int p_id) {
    if (p_id < 0 || p_id >= possessions.size()) {
        return;
    }
    godot::Ref<Prop> possession = possessions[p_id];
    if (_can_record_undo()) {
        undo_redo_->create_action("Character Lose Possession");
        undo_redo_->add_do_method(callable_mp(this, &Character::_remove_possession_at_direct).bind(p_id));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_insert_possession_at_direct).bind(p_id, possession));
        undo_redo_->commit_action();
    } else {
        _remove_possession_at_direct(p_id);
    }
}

// ----------------------------------------------------------------------------
// GOAL MANAGEMENT
// ----------------------------------------------------------------------------

int Character::set_goal(const godot::Ref<Plot> &p_goal) {
    if (!p_goal.is_valid()) {
        return -1;
    }
    int goal_id = get_goal_id(p_goal);
    if (goal_id < 0) {
        const int target_idx = static_cast<int>(goals.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Add Character Goal");
            undo_redo_->add_do_method(callable_mp(this, &Character::_insert_goal_at_direct).bind(target_idx, p_goal));
            undo_redo_->add_undo_method(callable_mp(this, &Character::_remove_goal_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_goal_at_direct(target_idx, p_goal);
        }
        goal_id = get_goal_id(p_goal);
    }
    return goal_id;
}

int Character::get_goal_id(const godot::Ref<Plot> &p_goal) const {
    return goals.find(p_goal);
}

bool Character::has_goal(const godot::Ref<Plot> &p_goal) const {
    return goals.has(p_goal);
}

void Character::remove_goal(const godot::Ref<Plot> &p_goal) {
    remove_goal_at(get_goal_id(p_goal));
}

void Character::remove_goal_at(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) {
        return;
    }
    godot::Ref<Plot> goal = goals[p_goal_id];
    if (_can_record_undo()) {
        undo_redo_->create_action("Remove Character Goal");
        undo_redo_->add_do_method(callable_mp(this, &Character::_remove_goal_at_direct).bind(p_goal_id));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_insert_goal_at_direct).bind(p_goal_id, goal));
        undo_redo_->commit_action();
    } else {
        _remove_goal_at_direct(p_goal_id);
    }
}

int Character::check_goal_status(const godot::Ref<Plot> &p_goal) {
    return check_goal_status_at(get_goal_id(p_goal));
}

int Character::check_goal_status_at(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) {
        return static_cast<int>(PLOT_CANNOT_COMPLETE);
    }

    godot::Ref<Plot> goal = goals[p_goal_id];
    if (!goal.is_valid()) {
        return static_cast<int>(PLOT_CANNOT_COMPLETE);
    }

    PlotStatus status = goal->get_status();
    if (status != PLOT_IN_PROGRESS && status != PLOT_NOT_BEGUN) {
        return static_cast<int>(status);
    }

    status = goal->evaluate_plot_status();
    switch (status) {
        case PLOT_CANNOT_COMPLETE:
        case PLOT_FAILED:
            goal_fail(p_goal_id);
            break;
        case PLOT_COMPLETE:
            goal_success(p_goal_id);
            break;
        default:
            break;
    }

    return static_cast<int>(status);
}

void Character::goal_success(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) return;
    emit_signal("goal_achieved", goals[p_goal_id]);
}

void Character::goal_fail(int p_goal_id) {
    if (p_goal_id < 0 || p_goal_id >= goals.size()) return;
    emit_signal("goal_failed", goals[p_goal_id]);
}

// ----------------------------------------------------------------------------
// RELATIONSHIP MANAGEMENT
// ----------------------------------------------------------------------------

int Character::begin_relationship(const godot::Ref<Narreme> &p_relation, const godot::String &p_title) {
    int id = get_relationship_id(p_title);
    if (id < 0) {
        godot::Ref<Relationship> new_relationship;
        new_relationship.instantiate();
        new_relationship->set_relation(p_relation);
        new_relationship->set_title(p_title);
        if (undo_redo_.is_valid()) {
            new_relationship->set_undo_redo(undo_redo_);
        }

        const int target_idx = static_cast<int>(relationships.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Begin Relationship");
            undo_redo_->add_do_method(callable_mp(this, &Character::_insert_relationship_at_direct).bind(target_idx, new_relationship));
            undo_redo_->add_undo_method(callable_mp(this, &Character::_remove_relationship_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_relationship_at_direct(target_idx, new_relationship);
        }
        id = get_relationship_id(p_title);
    } else {
        godot::Ref<Relationship> existing_rel = relationships[id];
        if (existing_rel.is_valid() && existing_rel->get_relation() != p_relation) {
            if (!change_relation(id, p_relation)) {
                return -1;
            }
        }
    }
    return id;
}

bool Character::end_relationship(const godot::Ref<Relationship> &p_relationship) {
    if (!p_relationship.is_valid()) {
        return false;
    }
    return end_relationship_at(relationships.find(p_relationship));
}

bool Character::end_relationship_at(int p_id) {
    if (p_id < 0 || p_id >= relationships.size()) {
        return false;
    }
    godot::Ref<Relationship> rel = relationships[p_id];
    if (_can_record_undo()) {
        undo_redo_->create_action("End Relationship");
        undo_redo_->add_do_method(callable_mp(this, &Character::_remove_relationship_at_direct).bind(p_id));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_insert_relationship_at_direct).bind(p_id, rel));
        undo_redo_->commit_action();
    } else {
        _remove_relationship_at_direct(p_id);
    }
    return true;
}

bool Character::change_relation(int p_id, const godot::Ref<Narreme> &p_new_relation) {
    if (p_id < 0 || p_id >= relationships.size()) return false;

    godot::Ref<Relationship> rel = relationships[p_id];
    if (!rel.is_valid()) return false;
    godot::Ref<Narreme> old_relation = rel->get_relation();
    if (old_relation == p_new_relation) return false;

    if (_can_record_undo()) {
        undo_redo_->create_action("Change Relationship Target");
        undo_redo_->add_do_method(callable_mp(this, &Character::_set_relationship_relation_direct).bind(p_id, p_new_relation));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_set_relationship_relation_direct).bind(p_id, old_relation));
        undo_redo_->commit_action();
    } else {
        _set_relationship_relation_direct(p_id, p_new_relation);
    }
    return true;
}

bool Character::change_relationship(int p_id, const godot::String &p_new_title) {
    if (p_id < 0 || p_id >= relationships.size()) return false;

    godot::Ref<Relationship> rel = relationships[p_id];
    if (!rel.is_valid()) return false;
    const godot::String old_title = rel->get_title();
    if (old_title == p_new_title) return false;

    if (_can_record_undo()) {
        undo_redo_->create_action("Rename Relationship");
        undo_redo_->add_do_method(callable_mp(this, &Character::_set_relationship_title_direct).bind(p_id, p_new_title));
        undo_redo_->add_undo_method(callable_mp(this, &Character::_set_relationship_title_direct).bind(p_id, old_title));
        undo_redo_->commit_action();
    } else {
        _set_relationship_title_direct(p_id, p_new_title);
    }
    return true;
}

int Character::get_relationship_id(const godot::String &p_title) const {
    for (int i = 0; i < relationships.size(); i++) {
        godot::Ref<Relationship> rel = relationships[i];
        if (rel.is_valid() && rel->get_title() == p_title) {
            return i;
        }
    }
    return -1;
}

godot::TypedArray<int> Character::get_relation_ids(const godot::Ref<Narreme> &p_target) const {
    godot::TypedArray<int> ids;
    for (int i = 0; i < relationships.size(); i++) {
        godot::Ref<Relationship> rel = relationships[i];
        if (rel.is_valid() && rel->get_relation() == p_target) {
            ids.append(i);
        }
    }
    return ids;
}

// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES (SEMANTIC + BACKWARD COMPATIBLE)
// ----------------------------------------------------------------------------

godot::TypedArray<godot::Dictionary> Character::get_narrative_condition_descriptors(Narreme *p_narreme) const {
    godot::TypedArray<godot::Dictionary> out;

    auto make_tags = [](std::initializer_list<const char *> p_list) {
        godot::PackedStringArray tags;
        for (const char *t : p_list) {
            tags.append(godot::String(t));
        }
        return tags;
    };

    if (p_narreme != nullptr) {
        const godot::String class_name = p_narreme->get_class_name_str();

        if (class_name == "Character") {
            out.append(make_condition_descriptor(
                0, "character.is_same", "is",
                "Identity", "Entity", "Compare_entity",
                make_tags({"identity", "character"}), true, "Character"
            ));
            out.append(make_condition_descriptor(
                1, "character.co_located_with", "is at the same location as",
                "Locative_relation", "Figure", "Ground",
                make_tags({"spatial", "co_location", "character"}), true, "Character"
            ));
            out.append(make_condition_descriptor(
                2, "character.shares_goal_with", "has the same goal as",
                "Desiring", "Experiencer", "Co_experiencer",
                make_tags({"intentionality", "goal", "shared"}), true, "Character"
            ));
            out.append(make_condition_descriptor(
                3, "character.related_to_character", "has a relationship with",
                "Personal_relationship", "Partner_1", "Partner_2",
                make_tags({"social", "relation", "character"}), true, "Character"
            ));
        } else if (class_name == "Prop") {
            out.append(make_condition_descriptor(
                0, "character.possesses", "possesses",
                "Possession", "Owner", "Possession",
                make_tags({"possession", "ownership", "prop"}), true, "Prop"
            ));
            out.append(make_condition_descriptor(
                1, "character.related_to_prop", "has a relationship with",
                "Personal_relationship", "Experiencer", "Topic",
                make_tags({"social", "relation", "prop"}), true, "Prop"
            ));
        } else if (class_name == "Location") {
            out.append(make_condition_descriptor(
                0, "character.located_at", "is at",
                "Locative_relation", "Figure", "Ground",
                make_tags({"spatial", "location", "presence"}), true, "Location"
            ));
            out.append(make_condition_descriptor(
                1, "character.related_to_location", "has a relationship with",
                "Personal_relationship", "Experiencer", "Topic",
                make_tags({"social", "relation", "spatial"}), true, "Location"
            ));
        } else if (class_name == "Incident") {
            out.append(make_condition_descriptor(
                0, "character.related_to_incident", "has a relationship with",
                "Personal_relationship", "Experiencer", "Event",
                make_tags({"social", "relation", "event"}), true, "Incident"
            ));
        } else if (class_name == "Plot") {
            out.append(make_condition_descriptor(
                0, "character.has_goal", "has goal",
                "Desiring", "Experiencer", "Event",
                make_tags({"intentionality", "goal", "plot"}), true, "Plot"
            ));
            out.append(make_condition_descriptor(
                1, "character.related_to_plot", "has a relationship with",
                "Personal_relationship", "Experiencer", "Topic",
                make_tags({"social", "relation", "plot"}), true, "Plot"
            ));
        } else if (class_name == "Narrative") {
            out.append(make_condition_descriptor(
                0, "character.related_to_narrative", "has a relationship with",
                "Personal_relationship", "Experiencer", "Topic",
                make_tags({"social", "relation", "narrative"}), true, "Narrative"
            ));
        }
    } else {
        out.append(make_condition_descriptor(
            0, "character.is_somewhere", "is somewhere.",
            "Locative_relation", "Figure", "",
            make_tags({"spatial", "location", "presence"}), false
        ));
        out.append(make_condition_descriptor(
            1, "character.has_possessions", "has possessions.",
            "Possession", "Owner", "",
            make_tags({"possession", "ownership"}), false
        ));
        out.append(make_condition_descriptor(
            2, "character.has_relationships", "has relationships.",
            "Personal_relationship", "Partner_1", "",
            make_tags({"social", "relation"}), false
        ));
        out.append(make_condition_descriptor(
            3, "character.has_goals", "has goals.",
            "Desiring", "Experiencer", "",
            make_tags({"intentionality", "goal"}), false
        ));
    }

    return out;
}

godot::Array Character::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Character::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        const godot::String class_name = p_conditional_narreme->get_class_name_str();

        if (class_name == "Character") {
            Character *conditional_character = godot::Object::cast_to<Character>(p_conditional_narreme);
            if (!conditional_character) return NarremeConditionStatus::UNKNOWN;

            switch (p_condition_id) {
                case 0:
                    return (conditional_character == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_character->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 2: {
                    godot::TypedArray<Plot> other_goals = conditional_character->get_goals();
                    for (int i = 0; i < goals.size(); i++) {
                        for (int j = 0; j < other_goals.size(); j++) {
                            if (goals[i] == other_goals[j]) {
                                return NarremeConditionStatus::MET;
                            }
                        }
                    }
                    return NarremeConditionStatus::NOT_MET;
                }
                case 3:
                    return (get_relation_ids(conditional_character).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Prop") {
            godot::Ref<Prop> conditional_prop(godot::Object::cast_to<Prop>(p_conditional_narreme));
            if (!conditional_prop.is_valid()) return NarremeConditionStatus::UNKNOWN;
            switch (p_condition_id) {
                case 0:
                    return has_possession_of(conditional_prop) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Location") {
            godot::Ref<Location> conditional_location(godot::Object::cast_to<Location>(p_conditional_narreme));
            if (!conditional_location.is_valid()) return NarremeConditionStatus::UNKNOWN;
            switch (p_condition_id) {
                case 0:
                    return (conditional_location == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Incident") {
            switch (p_condition_id) {
                case 0:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Plot") {
            godot::Ref<Plot> conditional_plot(godot::Object::cast_to<Plot>(p_conditional_narreme));
            if (!conditional_plot.is_valid()) return NarremeConditionStatus::UNKNOWN;
            switch (p_condition_id) {
                case 0:
                    return (get_goal_id(conditional_plot) >= 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Narrative") {
            switch (p_condition_id) {
                case 0:
                    return (get_relation_ids(p_conditional_narreme).size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        switch (p_condition_id) {
            case 0:
                return current_location.is_valid() ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (possessions.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (relationships.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 3:
                return (goals.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }

    return NarremeConditionStatus::UNKNOWN;
}

godot::String Character::get_class_name_str() const {
    return "Character";
}

} // namespace ideam::godot_ext