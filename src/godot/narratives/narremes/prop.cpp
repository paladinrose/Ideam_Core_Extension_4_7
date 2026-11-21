#include "prop.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "character.h"
#include "location.h"

namespace ideam::godot_ext {

void Prop::_bind_methods() {
    using namespace godot;

    // Properties
    ClassDB::bind_method(D_METHOD("set_possessor", "possessor"), &Prop::set_possessor);
    ClassDB::bind_method(D_METHOD("get_possessor"), &Prop::get_possessor);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "possessor", PROPERTY_HINT_RESOURCE_TYPE, "Character"), "set_possessor", "get_possessor");

    ClassDB::bind_method(D_METHOD("set_current_location", "current_location"), &Prop::set_current_location);
    ClassDB::bind_method(D_METHOD("get_current_location"), &Prop::get_current_location);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "current_location", PROPERTY_HINT_RESOURCE_TYPE, "Location"), "set_current_location", "get_current_location");

    // Methods
    ClassDB::bind_method(D_METHOD("taken_by", "new_possessor"), &Prop::taken_by);
    ClassDB::bind_method(D_METHOD("released_by", "old_possessor"), &Prop::released_by);
    ClassDB::bind_method(D_METHOD("become_lost"), &Prop::become_lost);
    ClassDB::bind_method(D_METHOD("found_by", "finder"), &Prop::found_by);
    ClassDB::bind_method(D_METHOD("enter_location", "new_location"), &Prop::enter_location);
    ClassDB::bind_method(D_METHOD("leave_current_location"), &Prop::leave_current_location);
    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Prop::get_class_name_str);

    // Signals
    ADD_SIGNAL(MethodInfo("taken", PropertyInfo(Variant::OBJECT, "takenBy", PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(MethodInfo("released", PropertyInfo(Variant::OBJECT, "releasedBy", PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(MethodInfo("became_lost"));
    ADD_SIGNAL(MethodInfo("found", PropertyInfo(Variant::OBJECT, "foundBy", PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(MethodInfo("location_entered", PropertyInfo(Variant::OBJECT, "new_location", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(MethodInfo("location_exited", PropertyInfo(Variant::OBJECT, "previous_location", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
}

Prop::Prop() = default;
Prop::~Prop() = default;

void Prop::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Prop::set_possessor(const godot::Ref<Character> &p_possessor) {
    possessor = p_possessor;
    emit_changed();
}
godot::Ref<Character> Prop::get_possessor() const { return possessor; }

void Prop::set_current_location(const godot::Ref<Location> &p_location) {
    current_location = p_location;
    emit_changed();
}
godot::Ref<Location> Prop::get_current_location() const { return current_location; }

// ----------------------------------------------------------------------------
// DIRECT INTERNAL MUTATORS
// ----------------------------------------------------------------------------

void Prop::_apply_possessor_direct(const godot::Ref<Character> &p_new_possessor, bool p_as_lost) {
    const godot::Ref<Character> old_possessor = possessor;
    if (!p_new_possessor.is_valid()) {
        possessor.unref();
        if (old_possessor.is_valid()) {
            emit_signal("released", old_possessor);
        }
        if (p_as_lost) {
            emit_signal("became_lost");
        }
    } else if (!old_possessor.is_valid()) {
        possessor = p_new_possessor;
        emit_signal("found", possessor);
    } else if (old_possessor != p_new_possessor) {
        possessor = p_new_possessor;
        emit_signal("taken", possessor);
    }
    emit_changed();
}

void Prop::_apply_location_direct(const godot::Ref<Location> &p_new_location) {
    if (p_new_location == current_location) {
        return;
    }
    if (current_location.is_valid()) {
        godot::Ref<Location> old_loc = current_location;
        current_location.unref();
        emit_signal("location_exited", old_loc);
    }
    if (p_new_location.is_valid()) {
        current_location = p_new_location;
        emit_signal("location_entered", current_location);
    }
    emit_changed();
}

// ----------------------------------------------------------------------------
// POSSESSION MANAGEMENT
// ----------------------------------------------------------------------------

void Prop::taken_by(const godot::Ref<Character> &new_possessor) {
    if (!possessor.is_valid()) {
        found_by(new_possessor);
        return;
    }
    if (possessor == new_possessor) {
        return;
    }
    if (_can_record_undo()) {
        const godot::Ref<Character> old_possessor = possessor;
        undo_redo_->create_action("Prop Taken By Character");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(new_possessor, false));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(old_possessor, false));
        undo_redo_->commit_action();
    } else {
        _apply_possessor_direct(new_possessor, false);
    }
}

void Prop::released_by(const godot::Ref<Character> &old_possessor) {
    if (old_possessor != possessor || !possessor.is_valid()) {
        return;
    }
    if (_can_record_undo()) {
        undo_redo_->create_action("Prop Released By Character");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(godot::Ref<Character>(), false));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(old_possessor, false));
        undo_redo_->commit_action();
    } else {
        _apply_possessor_direct(godot::Ref<Character>(), false);
    }
}

void Prop::become_lost() {
    if (_can_record_undo()) {
        const godot::Ref<Character> old_possessor = possessor;
        undo_redo_->create_action("Prop Become Lost");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(godot::Ref<Character>(), true));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(old_possessor, false));
        undo_redo_->commit_action();
    } else {
        _apply_possessor_direct(godot::Ref<Character>(), true);
    }
}

void Prop::found_by(const godot::Ref<Character> &finder) {
    if (_can_record_undo()) {
        const godot::Ref<Character> old_possessor = possessor;
        undo_redo_->create_action("Prop Found By Character");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(finder, false));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_possessor_direct).bind(old_possessor, !old_possessor.is_valid()));
        undo_redo_->commit_action();
    } else {
        _apply_possessor_direct(finder, false);
    }
}

// ----------------------------------------------------------------------------
// LOCATION MANAGEMENT
// ----------------------------------------------------------------------------

void Prop::enter_location(const godot::Ref<Location> &new_location) {
    if (new_location == current_location) return;
    if (_can_record_undo()) {
        const godot::Ref<Location> old_location = current_location;
        undo_redo_->create_action("Prop Enter Location");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_location_direct).bind(new_location));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_location_direct).bind(old_location));
        undo_redo_->commit_action();
    } else {
        _apply_location_direct(new_location);
    }
}

void Prop::leave_current_location() {
    if (!current_location.is_valid()) return;
    if (_can_record_undo()) {
        const godot::Ref<Location> old_location = current_location;
        undo_redo_->create_action("Prop Leave Location");
        undo_redo_->add_do_method(callable_mp(this, &Prop::_apply_location_direct).bind(godot::Ref<Location>()));
        undo_redo_->add_undo_method(callable_mp(this, &Prop::_apply_location_direct).bind(old_location));
        undo_redo_->commit_action();
    } else {
        _apply_location_direct(godot::Ref<Location>());
    }
}

// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES (SEMANTIC + BACKWARD COMPATIBLE)
// ----------------------------------------------------------------------------

godot::TypedArray<godot::Dictionary> Prop::get_narrative_condition_descriptors(Narreme *p_narreme) const {
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
                0, "prop.possessed_by", "is possessed by",
                "Possession", "Possession", "Owner",
                make_tags({"possession", "ownership", "prop"}), true, "Character"
            ));
            out.append(make_condition_descriptor(
                1, "prop.co_located_with_character", "is at the same location as",
                "Locative_relation", "Figure", "Ground",
                make_tags({"spatial", "co_location", "prop"}), true, "Character"
            ));
        } else if (class_name == "Prop") {
            out.append(make_condition_descriptor(
                0, "prop.is_same", "is",
                "Identity", "Entity", "Compare_entity",
                make_tags({"identity", "prop"}), true, "Prop"
            ));
            out.append(make_condition_descriptor(
                1, "prop.shares_possessor_with", "has the same possessor as",
                "Possession", "Possession", "Co_possession",
                make_tags({"possession", "ownership", "shared"}), true, "Prop"
            ));
            out.append(make_condition_descriptor(
                2, "prop.co_located_with_prop", "is at the same location as",
                "Locative_relation", "Figure", "Ground",
                make_tags({"spatial", "co_location", "prop"}), true, "Prop"
            ));
        } else if (class_name == "Location") {
            out.append(make_condition_descriptor(
                0, "prop.located_at", "is at",
                "Locative_relation", "Figure", "Ground",
                make_tags({"spatial", "location", "prop"}), true, "Location"
            ));
        }
    } else {
        out.append(make_condition_descriptor(
            0, "prop.is_lost", "is lost",
            "Possession", "Possession", "",
            make_tags({"possession", "lost", "unlocated"}), false
        ));
        out.append(make_condition_descriptor(
            1, "prop.has_possessor", "has possessor",
            "Possession", "Possession", "",
            make_tags({"possession", "ownership"}), false
        ));
        out.append(make_condition_descriptor(
            2, "prop.is_somewhere", "is somewhere",
            "Locative_relation", "Figure", "",
            make_tags({"spatial", "location"}), false
        ));
    }

    return out;
}

godot::Array Prop::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Prop::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        const godot::String class_name = p_conditional_narreme->get_class_name_str();

        if (class_name == "Character") {
            Character *conditional_character = godot::Object::cast_to<Character>(p_conditional_narreme);
            if (!conditional_character) return NarremeConditionStatus::UNKNOWN;

            switch (p_condition_id) {
                case 0:
                    return (possessor.ptr() == conditional_character) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_character->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Prop") {
            Prop *conditional_prop = godot::Object::cast_to<Prop>(p_conditional_narreme);
            if (!conditional_prop) return NarremeConditionStatus::UNKNOWN;

            switch (p_condition_id) {
                case 0:
                    return (conditional_prop == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return (conditional_prop->get_possessor() == possessor) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 2:
                    return (conditional_prop->get_current_location() == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Location") {
            godot::Ref<Location> conditional_location(godot::Object::cast_to<Location>(p_conditional_narreme));
            if (!conditional_location.is_valid()) return NarremeConditionStatus::UNKNOWN;

            switch (p_condition_id) {
                case 0:
                    return (conditional_location == current_location) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        switch (p_condition_id) {
            case 0:
                return (!possessor.is_valid() && !current_location.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (possessor.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (current_location.is_valid()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }

    return NarremeConditionStatus::UNKNOWN;
}

godot::String Prop::get_class_name_str() const {
    return "Prop";
}

} // namespace ideam::godot_ext