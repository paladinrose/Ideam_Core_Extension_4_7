#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include "location.h"
#include "character.h"
#include "prop.h"

#include <algorithm>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ideam::godot_ext {

void Location::_bind_methods() {
    using namespace godot;

    // Properties
    ClassDB::bind_method(D_METHOD("set_characters", "characters"), &Location::set_characters);
    ClassDB::bind_method(D_METHOD("get_characters"), &Location::get_characters);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "characters", PROPERTY_HINT_ARRAY_TYPE, "Character"), "set_characters", "get_characters");

    ClassDB::bind_method(D_METHOD("set_props", "props"), &Location::set_props);
    ClassDB::bind_method(D_METHOD("get_props"), &Location::get_props);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "props", PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_props", "get_props");

    ClassDB::bind_method(D_METHOD("set_connected_locations", "connected_locations"), &Location::set_connected_locations);
    ClassDB::bind_method(D_METHOD("get_connected_locations"), &Location::get_connected_locations);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "connected_locations", PROPERTY_HINT_ARRAY_TYPE, "Location"),
        "set_connected_locations",
        "get_connected_locations"
    );

    // Character Methods
    ClassDB::bind_method(D_METHOD("character_entrance", "character"), &Location::character_entrance);
    ClassDB::bind_method(D_METHOD("get_character_id", "character"), &Location::get_character_id);
    ClassDB::bind_method(D_METHOD("has_character", "character"), &Location::has_character);
    ClassDB::bind_method(D_METHOD("character_exit", "character"), &Location::character_exit);
    ClassDB::bind_method(D_METHOD("character_exit_at", "character_ID"), &Location::character_exit_at);

    // Prop Methods
    ClassDB::bind_method(D_METHOD("prop_entrance", "prop"), &Location::prop_entrance);
    ClassDB::bind_method(D_METHOD("get_prop_id", "prop"), &Location::get_prop_id);
    ClassDB::bind_method(D_METHOD("has_prop", "prop"), &Location::has_prop);
    ClassDB::bind_method(D_METHOD("prop_exit", "prop"), &Location::prop_exit);
    ClassDB::bind_method(D_METHOD("prop_exit_at", "prop_ID"), &Location::prop_exit_at);

    // Topology Methods
    ClassDB::bind_method(D_METHOD("connect_location", "target", "bidirectional"), &Location::connect_location, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("disconnect_location", "target", "bidirectional"), &Location::disconnect_location, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("disconnect_location_at", "connection_id"), &Location::disconnect_location_at);
    ClassDB::bind_method(D_METHOD("get_connected_location_id", "target"), &Location::get_connected_location_id);
    ClassDB::bind_method(D_METHOD("is_connected_to", "target"), &Location::is_connected_to);
    ClassDB::bind_method(D_METHOD("has_connections"), &Location::has_connections);
    ClassDB::bind_method(D_METHOD("get_connection_count"), &Location::get_connection_count);

    // Pathfinding Methods
    ClassDB::bind_method(D_METHOD("find_path_to", "target", "max_depth"), &Location::find_path_to, DEFVAL(64));
    ClassDB::bind_method(D_METHOD("has_path_to", "target", "max_depth"), &Location::has_path_to, DEFVAL(64));
    ClassDB::bind_method(D_METHOD("get_distance_to", "target", "max_depth"), &Location::get_distance_to, DEFVAL(64));
    ClassDB::bind_method(D_METHOD("get_reachable_locations", "max_depth"), &Location::get_reachable_locations, DEFVAL(64));

    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Location::get_class_name_str);

    // Signals
    ADD_SIGNAL(MethodInfo("character_entered", PropertyInfo(Variant::OBJECT, "character", PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(MethodInfo("character_exited", PropertyInfo(Variant::OBJECT, "character", PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(MethodInfo("prop_entered", PropertyInfo(Variant::OBJECT, "prop", PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(MethodInfo("prop_exited", PropertyInfo(Variant::OBJECT, "prop", PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(MethodInfo("location_connected", PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(MethodInfo("location_disconnected", PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
}

Location::Location() = default;
Location::~Location() = default;

void Location::initialize() {
    Narreme::initialize();
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS
// ----------------------------------------------------------------------------

void Location::set_characters(const godot::TypedArray<Character> &p_characters) {
    characters = p_characters;
    emit_changed();
}

godot::TypedArray<Character> Location::get_characters() const { return characters; }

void Location::set_props(const godot::TypedArray<Prop> &p_props) {
    props = p_props;
    emit_changed();
}

godot::TypedArray<Prop> Location::get_props() const { return props; }

void Location::set_connected_locations(const godot::TypedArray<Location> &p_locations) {
    connected_locations = p_locations;
    emit_changed();
}

godot::TypedArray<Location> Location::get_connected_locations() const { return connected_locations; }

// ----------------------------------------------------------------------------
// DIRECT INTERNAL MUTATORS
// ----------------------------------------------------------------------------

void Location::_insert_character_at_direct(int p_index, const godot::Ref<Character> &p_character) {
    if (!p_character.is_valid() || characters.has(p_character)) {
        return;
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(characters.size()));
    characters.insert(safe_idx, p_character);
    emit_signal("character_entered", p_character);
    emit_changed();
}

void Location::_remove_character_at_direct(int p_index) {
    if (p_index < 0 || p_index >= characters.size()) {
        return;
    }
    godot::Ref<Character> removed = characters[p_index];
    characters.remove_at(p_index);
    emit_signal("character_exited", removed);
    emit_changed();
}

void Location::_insert_prop_at_direct(int p_index, const godot::Ref<Prop> &p_prop) {
    if (!p_prop.is_valid() || props.has(p_prop)) {
        return;
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(props.size()));
    props.insert(safe_idx, p_prop);
    emit_signal("prop_entered", p_prop);
    emit_changed();
}

void Location::_remove_prop_at_direct(int p_index) {
    if (p_index < 0 || p_index >= props.size()) {
        return;
    }
    godot::Ref<Prop> removed = props[p_index];
    props.remove_at(p_index);
    emit_signal("prop_exited", removed);
    emit_changed();
}

void Location::_insert_connected_location_at_direct(int p_index, const godot::Ref<Location> &p_target) {
    if (!p_target.is_valid() || p_target.ptr() == this || connected_locations.has(p_target)) {
        return;
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(connected_locations.size()));
    connected_locations.insert(safe_idx, p_target);
    emit_signal("location_connected", p_target);
    emit_changed();
}

void Location::_remove_connected_location_at_direct(int p_index) {
    if (p_index < 0 || p_index >= connected_locations.size()) {
        return;
    }
    godot::Ref<Location> removed = connected_locations[p_index];
    connected_locations.remove_at(p_index);
    emit_signal("location_disconnected", removed);
    emit_changed();
}

void Location::_remove_connected_location_direct(const godot::Ref<Location> &p_target) {
    const int idx = get_connected_location_id(p_target);
    if (idx >= 0) {
        _remove_connected_location_at_direct(idx);
    }
}

// ----------------------------------------------------------------------------
// CHARACTER MANAGEMENT
// ----------------------------------------------------------------------------

int Location::character_entrance(const godot::Ref<Character> &character) {
    if (!character.is_valid()) {
        return -1;
    }
    int character_ID = get_character_id(character);
    if (character_ID < 0) {
        const int target_idx = static_cast<int>(characters.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Character Enter Location");
            undo_redo_->add_do_method(callable_mp(this, &Location::_insert_character_at_direct).bind(target_idx, character));
            undo_redo_->add_undo_method(callable_mp(this, &Location::_remove_character_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_character_at_direct(target_idx, character);
        }
        character_ID = get_character_id(character);
    }
    return character_ID;
}

int Location::get_character_id(const godot::Ref<Character> &character) const {
    return characters.find(character);
}

bool Location::has_character(const godot::Ref<Character> &character) const {
    return characters.has(character);
}

bool Location::character_exit(const godot::Ref<Character> &character) {
    return character_exit_at(get_character_id(character));
}

bool Location::character_exit_at(int character_ID) {
    if (character_ID < 0 || character_ID >= characters.size()) {
        return false;
    }
    godot::Ref<Character> to_remove = characters[character_ID];
    if (_can_record_undo()) {
        undo_redo_->create_action("Character Exit Location");
        undo_redo_->add_do_method(callable_mp(this, &Location::_remove_character_at_direct).bind(character_ID));
        undo_redo_->add_undo_method(callable_mp(this, &Location::_insert_character_at_direct).bind(character_ID, to_remove));
        undo_redo_->commit_action();
    } else {
        _remove_character_at_direct(character_ID);
    }
    return true;
}

// ----------------------------------------------------------------------------
// PROP MANAGEMENT
// ----------------------------------------------------------------------------

int Location::prop_entrance(const godot::Ref<Prop> &prop) {
    if (!prop.is_valid()) {
        return -1;
    }
    int prop_ID = get_prop_id(prop);
    if (prop_ID < 0) {
        const int target_idx = static_cast<int>(props.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Prop Enter Location");
            undo_redo_->add_do_method(callable_mp(this, &Location::_insert_prop_at_direct).bind(target_idx, prop));
            undo_redo_->add_undo_method(callable_mp(this, &Location::_remove_prop_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_prop_at_direct(target_idx, prop);
        }
        prop_ID = get_prop_id(prop);
    }
    return prop_ID;
}

int Location::get_prop_id(const godot::Ref<Prop> &prop) const {
    return props.find(prop);
}

bool Location::has_prop(const godot::Ref<Prop> &prop) const {
    return props.has(prop);
}

bool Location::prop_exit(const godot::Ref<Prop> &prop) {
    return prop_exit_at(get_prop_id(prop));
}

bool Location::prop_exit_at(int prop_ID) {
    if (prop_ID < 0 || prop_ID >= props.size()) {
        return false;
    }
    godot::Ref<Prop> to_remove = props[prop_ID];
    if (_can_record_undo()) {
        undo_redo_->create_action("Prop Exit Location");
        undo_redo_->add_do_method(callable_mp(this, &Location::_remove_prop_at_direct).bind(prop_ID));
        undo_redo_->add_undo_method(callable_mp(this, &Location::_insert_prop_at_direct).bind(prop_ID, to_remove));
        undo_redo_->commit_action();
    } else {
        _remove_prop_at_direct(prop_ID);
    }
    return true;
}

// ----------------------------------------------------------------------------
// DIRECTED NARRATIVE TOPOLOGY MANAGEMENT
// ----------------------------------------------------------------------------

int Location::connect_location(const godot::Ref<Location> &p_target, bool p_bidirectional) {
    if (!p_target.is_valid() || p_target.ptr() == this) {
        return -1;
    }

    const bool need_forward = !is_connected_to(p_target);
    const bool need_reverse = p_bidirectional && !p_target->is_connected_to(godot::Ref<Location>(this));
    if (!need_forward && !need_reverse) {
        return get_connected_location_id(p_target);
    }

    const int fwd_idx = static_cast<int>(connected_locations.size());
    const int rev_idx = static_cast<int>(p_target->connected_locations.size());
    const godot::Ref<Location> self_ref(this);

    if (_can_record_undo()) {
        undo_redo_->create_action(p_bidirectional ? "Connect Locations (Bidirectional)" : "Connect Location");
        if (need_forward) {
            undo_redo_->add_do_method(callable_mp(this, &Location::_insert_connected_location_at_direct).bind(fwd_idx, p_target));
            undo_redo_->add_undo_method(callable_mp(this, &Location::_remove_connected_location_direct).bind(p_target));
        }
        if (need_reverse) {
            undo_redo_->add_do_method(callable_mp(p_target.ptr(), &Location::_insert_connected_location_at_direct).bind(rev_idx, self_ref));
            undo_redo_->add_undo_method(callable_mp(p_target.ptr(), &Location::_remove_connected_location_direct).bind(self_ref));
        }
        undo_redo_->commit_action();
    } else {
        if (need_forward) {
            _insert_connected_location_at_direct(fwd_idx, p_target);
        }
        if (need_reverse) {
            p_target->_insert_connected_location_at_direct(rev_idx, self_ref);
        }
    }

    return get_connected_location_id(p_target);
}

bool Location::disconnect_location(const godot::Ref<Location> &p_target, bool p_bidirectional) {
    if (!p_target.is_valid()) {
        return false;
    }

    const int fwd_idx = get_connected_location_id(p_target);
    const godot::Ref<Location> self_ref(this);
    const int rev_idx = p_bidirectional ? p_target->get_connected_location_id(self_ref) : -1;

    if (fwd_idx < 0 && rev_idx < 0) {
        return false;
    }

    if (_can_record_undo()) {
        undo_redo_->create_action(p_bidirectional ? "Disconnect Locations (Bidirectional)" : "Disconnect Location");
        if (fwd_idx >= 0) {
            undo_redo_->add_do_method(callable_mp(this, &Location::_remove_connected_location_direct).bind(p_target));
            undo_redo_->add_undo_method(callable_mp(this, &Location::_insert_connected_location_at_direct).bind(fwd_idx, p_target));
        }
        if (rev_idx >= 0) {
            undo_redo_->add_do_method(callable_mp(p_target.ptr(), &Location::_remove_connected_location_direct).bind(self_ref));
            undo_redo_->add_undo_method(callable_mp(p_target.ptr(), &Location::_insert_connected_location_at_direct).bind(rev_idx, self_ref));
        }
        undo_redo_->commit_action();
    } else {
        if (fwd_idx >= 0) {
            _remove_connected_location_at_direct(fwd_idx);
        }
        if (rev_idx >= 0) {
            p_target->_remove_connected_location_at_direct(rev_idx);
        }
    }

    return true;
}

bool Location::disconnect_location_at(int p_connection_id) {
    if (p_connection_id < 0 || p_connection_id >= connected_locations.size()) {
        return false;
    }
    godot::Ref<Location> target = connected_locations[p_connection_id];
    if (_can_record_undo()) {
        undo_redo_->create_action("Disconnect Location");
        undo_redo_->add_do_method(callable_mp(this, &Location::_remove_connected_location_at_direct).bind(p_connection_id));
        undo_redo_->add_undo_method(callable_mp(this, &Location::_insert_connected_location_at_direct).bind(p_connection_id, target));
        undo_redo_->commit_action();
    } else {
        _remove_connected_location_at_direct(p_connection_id);
    }
    return true;
}

int Location::get_connected_location_id(const godot::Ref<Location> &p_target) const {
    return connected_locations.find(p_target);
}

bool Location::is_connected_to(const godot::Ref<Location> &p_target) const {
    return connected_locations.has(p_target);
}

bool Location::has_connections() const {
    for (int i = 0; i < connected_locations.size(); ++i) {
        godot::Ref<Location> loc = connected_locations[i];
        if (loc.is_valid()) {
            return true;
        }
    }
    return false;
}

int Location::get_connection_count() const {
    int count = 0;
    for (int i = 0; i < connected_locations.size(); ++i) {
        godot::Ref<Location> loc = connected_locations[i];
        if (loc.is_valid()) {
            ++count;
        }
    }
    return count;
}

// ----------------------------------------------------------------------------
// DIRECTED GRAPH PATHFINDING (BFS)
// ----------------------------------------------------------------------------

godot::TypedArray<Location> Location::find_path_to(const godot::Ref<Location> &p_target, int p_max_depth) const {
    godot::TypedArray<Location> result;
    if (!p_target.is_valid() || p_max_depth < 0) {
        return result;
    }

    const Location *start_ptr  = this;
    const Location *target_ptr = p_target.ptr();
    if (start_ptr == target_ptr) {
        result.append(godot::Ref<Location>(const_cast<Location *>(start_ptr)));
        return result;
    }

    struct QueueEntry {
        const Location *node = nullptr;
        int depth            = 0;
    };

    std::queue<QueueEntry> frontier;
    std::unordered_set<uint64_t> visited;
    std::unordered_map<uint64_t, const Location *> parent_map;

    const uint64_t start_id  = start_ptr->get_instance_id();
    const uint64_t target_id = target_ptr->get_instance_id();

    frontier.push({start_ptr, 0});
    visited.insert(start_id);

    bool found = false;
    while (!frontier.empty()) {
        const QueueEntry current = frontier.front();
        frontier.pop();

        if (current.depth >= p_max_depth) {
            continue;
        }

        const godot::TypedArray<Location> &neighbors = current.node->connected_locations;
        for (int i = 0; i < neighbors.size(); ++i) {
            godot::Ref<Location> neighbor = neighbors[i];
            if (!neighbor.is_valid()) {
                continue;
            }
            const Location *neighbor_ptr = neighbor.ptr();
            const uint64_t neighbor_id   = neighbor_ptr->get_instance_id();
            if (visited.find(neighbor_id) != visited.end()) {
                continue;
            }

            visited.insert(neighbor_id);
            parent_map[neighbor_id] = current.node;

            if (neighbor_id == target_id) {
                found = true;
                break;
            }
            frontier.push({neighbor_ptr, current.depth + 1});
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        return result;
    }

    std::vector<const Location *> reversed_path;
    const Location *cursor = target_ptr;
    while (cursor != nullptr) {
        reversed_path.push_back(cursor);
        if (cursor == start_ptr) {
            break;
        }
        const auto it = parent_map.find(cursor->get_instance_id());
        cursor = (it != parent_map.end()) ? it->second : nullptr;
    }

    for (auto it = reversed_path.rbegin(); it != reversed_path.rend(); ++it) {
        result.append(godot::Ref<Location>(const_cast<Location *>(*it)));
    }
    return result;
}

bool Location::has_path_to(const godot::Ref<Location> &p_target, int p_max_depth) const {
    return !find_path_to(p_target, p_max_depth).is_empty();
}

int Location::get_distance_to(const godot::Ref<Location> &p_target, int p_max_depth) const {
    const godot::TypedArray<Location> path = find_path_to(p_target, p_max_depth);
    if (path.is_empty()) {
        return -1;
    }
    return static_cast<int>(path.size()) - 1;
}

godot::TypedArray<Location> Location::get_reachable_locations(int p_max_depth) const {
    godot::TypedArray<Location> reachable;
    if (p_max_depth <= 0) {
        return reachable;
    }

    struct QueueEntry {
        const Location *node = nullptr;
        int depth            = 0;
    };

    std::queue<QueueEntry> frontier;
    std::unordered_set<uint64_t> visited;

    frontier.push({this, 0});
    visited.insert(get_instance_id());

    while (!frontier.empty()) {
        const QueueEntry current = frontier.front();
        frontier.pop();

        if (current.depth >= p_max_depth) {
            continue;
        }

        const godot::TypedArray<Location> &neighbors = current.node->connected_locations;
        for (int i = 0; i < neighbors.size(); ++i) {
            godot::Ref<Location> neighbor = neighbors[i];
            if (!neighbor.is_valid()) {
                continue;
            }
            const uint64_t nid = neighbor->get_instance_id();
            if (visited.find(nid) != visited.end()) {
                continue;
            }
            visited.insert(nid);
            reachable.append(neighbor);
            frontier.push({neighbor.ptr(), current.depth + 1});
        }
    }

    return reachable;
}

// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES (SEMANTIC + BACKWARD COMPATIBLE)
// ----------------------------------------------------------------------------

godot::TypedArray<godot::Dictionary> Location::get_narrative_condition_descriptors(Narreme *p_narreme) const {
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
                0, "location.contains_character", "has character",
                "Containment", "Container", "Theme",
                make_tags({"spatial", "containment", "character"}), true, "Character"
            ));
        } else if (class_name == "Prop") {
            out.append(make_condition_descriptor(
                0, "location.contains_prop", "has prop",
                "Containment", "Container", "Theme",
                make_tags({"spatial", "containment", "prop"}), true, "Prop"
            ));
        } else if (class_name == "Location") {
            out.append(make_condition_descriptor(
                0, "location.is_same", "is",
                "Identity", "Entity", "Compare_entity",
                make_tags({"identity", "spatial"}), true, "Location"
            ));
            out.append(make_condition_descriptor(
                1, "location.connects_to", "connects to",
                "Spatial_connectivity", "Origin", "Destination",
                make_tags({"spatial", "connectivity", "directed"}), true, "Location"
            ));
            out.append(make_condition_descriptor(
                2, "location.has_path_to", "has a path to",
                "Path_reachability", "Origin", "Destination",
                make_tags({"spatial", "path", "reachability", "directed"}), true, "Location"
            ));
            out.append(make_condition_descriptor(
                3, "location.mutually_connected_with", "is mutually connected with",
                "Spatial_connectivity", "Origin", "Destination",
                make_tags({"spatial", "connectivity", "bidirectional"}), true, "Location"
            ));
        }
    } else {
        out.append(make_condition_descriptor(
            0, "location.is_empty", "is empty",
            "Containment", "Container", "",
            make_tags({"spatial", "containment", "empty"}), false
        ));
        out.append(make_condition_descriptor(
            1, "location.has_characters", "has characters present",
            "Containment", "Container", "",
            make_tags({"spatial", "containment", "character"}), false
        ));
        out.append(make_condition_descriptor(
            2, "location.has_props", "has props present",
            "Containment", "Container", "",
            make_tags({"spatial", "containment", "prop"}), false
        ));
        out.append(make_condition_descriptor(
            3, "location.has_connections", "has connections",
            "Spatial_connectivity", "Origin", "",
            make_tags({"spatial", "connectivity"}), false
        ));
        out.append(make_condition_descriptor(
            4, "location.is_isolated", "is isolated",
            "Spatial_connectivity", "Origin", "",
            make_tags({"spatial", "connectivity", "isolated"}), false
        ));
    }

    return out;
}

godot::Array Location::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Location::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        const godot::String class_name = p_conditional_narreme->get_class_name_str();

        if (class_name == "Character") {
            godot::Ref<Character> conditional_character(godot::Object::cast_to<Character>(p_conditional_narreme));
            if (!conditional_character.is_valid()) return NarremeConditionStatus::UNKNOWN;
            switch (p_condition_id) {
                case 0:
                    return has_character(conditional_character) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Prop") {
            godot::Ref<Prop> conditional_prop(godot::Object::cast_to<Prop>(p_conditional_narreme));
            if (!conditional_prop.is_valid()) return NarremeConditionStatus::UNKNOWN;
            switch (p_condition_id) {
                case 0:
                    return has_prop(conditional_prop) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Location") {
            Location *conditional_location = godot::Object::cast_to<Location>(p_conditional_narreme);
            if (!conditional_location) return NarremeConditionStatus::UNKNOWN;
            godot::Ref<Location> target_ref(conditional_location);

            switch (p_condition_id) {
                case 0:
                    return (conditional_location == this) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 1:
                    return is_connected_to(target_ref) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 2:
                    return has_path_to(target_ref) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
                case 3:
                    return (is_connected_to(target_ref) && conditional_location->is_connected_to(godot::Ref<Location>(const_cast<Location *>(this))))
                        ? NarremeConditionStatus::MET
                        : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        switch (p_condition_id) {
            case 0:
                return (characters.size() == 0 && props.size() == 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1:
                return (characters.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2:
                return (props.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 3:
                return has_connections() ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 4:
                return (!has_connections()) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }

    return NarremeConditionStatus::UNKNOWN;
}

godot::String Location::get_class_name_str() const {
    return "Location";
}

} // namespace ideam::godot_ext