#include "incident.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include "../helpers/incident_condition.h"
#include "../helpers/narrative_condition.h"

#include <algorithm>

namespace ideam::godot_ext {

void Incident::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_status", "status"), &Incident::set_status);
    ClassDB::bind_method(D_METHOD("get_status"), &Incident::get_status);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "status"), "set_status", "get_status");

    ClassDB::bind_method(D_METHOD("set_conditions", "conditions"), &Incident::set_conditions);
    ClassDB::bind_method(D_METHOD("get_conditions"), &Incident::get_conditions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_ARRAY_TYPE, "Incident_Condition"), "set_conditions", "get_conditions");

    ClassDB::bind_method(D_METHOD("set_semantic_frame", "semantic_frame"), &Incident::set_semantic_frame);
    ClassDB::bind_method(D_METHOD("get_semantic_frame"), &Incident::get_semantic_frame);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_frame", PROPERTY_HINT_RESOURCE_TYPE, "SemanticFrameResource"),
        "set_semantic_frame",
        "get_semantic_frame"
    );
    ClassDB::bind_method(
        D_METHOD("commit_semantic_frame", "frame", "action_name"),
        &Incident::commit_semantic_frame,
        DEFVAL(String("Set Incident Semantic Frame"))
    );

    ClassDB::bind_method(D_METHOD("evaluate_incident"), &Incident::evaluate_incident);
    ClassDB::bind_method(D_METHOD("gather_conditions"), &Incident::gather_conditions);
    ClassDB::bind_method(D_METHOD("create_narrative_condition", "object", "condition", "subject"), &Incident::create_narrative_condition, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("add_condition", "new_condition"), &Incident::add_condition);
    ClassDB::bind_method(D_METHOD("get_condition_id", "condition"), &Incident::get_condition_id);
    ClassDB::bind_method(D_METHOD("has_condition", "condition"), &Incident::has_condition);
    ClassDB::bind_method(D_METHOD("evaluate_condition", "id"), &Incident::evaluate_condition);
    ClassDB::bind_method(D_METHOD("remove_condition", "condition"), &Incident::remove_condition);
    ClassDB::bind_method(D_METHOD("remove_condition_at", "condition_ID"), &Incident::remove_condition_at);

    ClassDB::bind_method(D_METHOD("_narcon_subject_change", "narcon"), &Incident::_narcon_subject_change);
    ClassDB::bind_method(D_METHOD("_narcon_object_change", "narcon"), &Incident::_narcon_object_change);
    ClassDB::bind_method(D_METHOD("_narcon_condition_change", "narcon"), &Incident::_narcon_condition_change);

    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Incident::get_class_name_str);

    ADD_SIGNAL(MethodInfo("cannot_happen"));
    ADD_SIGNAL(MethodInfo("happened"));
    ADD_SIGNAL(MethodInfo("condition_added", PropertyInfo(Variant::OBJECT, "new_condition", PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition")));
    ADD_SIGNAL(MethodInfo("condition_removed", PropertyInfo(Variant::OBJECT, "removed_condition", PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition")));

    BIND_ENUM_CONSTANT(IncidentStatus::HAS_NOT_HAPPENED);
    BIND_ENUM_CONSTANT(IncidentStatus::HAS_HAPPENED);
    BIND_ENUM_CONSTANT(IncidentStatus::CANNOT_HAPPEN);
}

Incident::Incident() = default;
Incident::~Incident() = default;

void Incident::initialize() {
    Narreme::initialize();
    gather_conditions();
}

void Incident::set_status(IncidentStatus p_status) {
    status = p_status;
    emit_changed();
}
IncidentStatus Incident::get_status() const { return status; }

void Incident::set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions) {
    conditions = p_conditions;
    emit_changed();
}
godot::TypedArray<Incident_Condition> Incident::get_conditions() const { return conditions; }

void Incident::_set_semantic_frame_direct(const godot::Ref<SemanticFrameResource> &p_frame) {
    if (semantic_frame_ == p_frame) return;
    semantic_frame_ = p_frame;
    if (semantic_frame_.is_valid() && undo_redo_.is_valid()) {
        semantic_frame_->set_undo_redo(undo_redo_);
    }
    emit_changed();
}

void Incident::set_semantic_frame(const godot::Ref<SemanticFrameResource> &p_frame) {
    _set_semantic_frame_direct(p_frame);
}

void Incident::commit_semantic_frame(
    const godot::Ref<SemanticFrameResource> &p_frame,
    const godot::String &p_action_name
) {
    if (semantic_frame_ == p_frame) return;
    if (_can_record_undo()) {
        const godot::Ref<SemanticFrameResource> old_frame = semantic_frame_;
        undo_redo_->create_action(
            p_action_name.is_empty() ? godot::String("Set Incident Semantic Frame") : p_action_name
        );
        undo_redo_->add_do_method(callable_mp(this, &Incident::_set_semantic_frame_direct).bind(p_frame));
        undo_redo_->add_undo_method(callable_mp(this, &Incident::_set_semantic_frame_direct).bind(old_frame));
        undo_redo_->commit_action();
    } else {
        _set_semantic_frame_direct(p_frame);
    }
}

void Incident::_insert_condition_at_direct(int p_index, const godot::Ref<Incident_Condition> &p_condition) {
    if (!p_condition.is_valid() || conditions.has(p_condition)) {
        return;
    }
    if (undo_redo_.is_valid()) {
        p_condition->set_undo_redo(undo_redo_);
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(conditions.size()));
    conditions.insert(safe_idx, p_condition);
    _connect_to_condition(p_condition);
    emit_signal("condition_added", p_condition);
    emit_changed();
}

void Incident::_remove_condition_at_direct(int p_index) {
    if (p_index < 0 || p_index >= conditions.size()) {
        return;
    }
    godot::Ref<Incident_Condition> condition = conditions[p_index];
    _disconnect_from_condition(condition);
    conditions.remove_at(p_index);
    emit_signal("condition_removed", condition);
    emit_changed();
}

void Incident::evaluate_incident() {
    if (status == IncidentStatus::HAS_HAPPENED || status == IncidentStatus::CANNOT_HAPPEN) return;

    IncidentStatus new_status = IncidentStatus::HAS_HAPPENED;

    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (!condition.is_valid()) continue;

        if (condition->get_last_status() == NarremeConditionStatus::UNKNOWN) {
            condition->evaluate_condition();
        }

        if (condition->get_last_status() == NarremeConditionStatus::CANNOT_MEET) {
            status = IncidentStatus::CANNOT_HAPPEN;
            emit_signal("cannot_happen");
            return;
        }

        if (condition->get_last_status() == NarremeConditionStatus::NOT_MET) {
            new_status = IncidentStatus::HAS_NOT_HAPPENED;
        }
    }

    if (new_status == IncidentStatus::HAS_HAPPENED) {
        status = IncidentStatus::HAS_HAPPENED;
        emit_signal("happened");
    }
}

void Incident::gather_conditions() {
    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (condition.is_valid()) {
            _connect_to_condition(condition);
        }
    }
}

int Incident::create_narrative_condition(const godot::Ref<Narreme> &object, int condition, const godot::Ref<Narreme> &subject) {
    godot::Ref<Narrative_Condition> newCondition;
    newCondition.instantiate();

    newCondition->set_object(object);
    newCondition->set_condition(condition);
    newCondition->set_subject(subject);
    newCondition->sync_semantic_predicate_key();
    if (undo_redo_.is_valid()) {
        newCondition->set_undo_redo(undo_redo_);
    }

    return add_condition(newCondition);
}

int Incident::add_condition(const godot::Ref<Incident_Condition> &new_condition) {
    if (!new_condition.is_valid()) return -1;

    int id = get_condition_id(new_condition);
    if (id < 0) {
        const int target_idx = static_cast<int>(conditions.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Add Incident Condition");
            undo_redo_->add_do_method(callable_mp(this, &Incident::_insert_condition_at_direct).bind(target_idx, new_condition));
            undo_redo_->add_undo_method(callable_mp(this, &Incident::_remove_condition_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_condition_at_direct(target_idx, new_condition);
        }
        id = get_condition_id(new_condition);
    }
    return id;
}

int Incident::get_condition_id(const godot::Ref<Incident_Condition> &condition) const {
    return conditions.find(condition);
}

bool Incident::has_condition(const godot::Ref<Incident_Condition> &condition) const {
    return conditions.has(condition);
}

NarremeConditionStatus Incident::evaluate_condition(int id) {
    if (id < 0 || id >= conditions.size()) return NarremeConditionStatus::CANNOT_MEET;

    godot::Ref<Incident_Condition> condition = conditions[id];
    if (condition.is_valid()) {
        return condition->evaluate_condition();
    }
    return NarremeConditionStatus::CANNOT_MEET;
}

bool Incident::remove_condition(const godot::Ref<Incident_Condition> &condition) {
    int condition_ID = get_condition_id(condition);
    if (condition_ID >= 0) return remove_condition_at(condition_ID);
    return false;
}

bool Incident::remove_condition_at(int condition_ID) {
    if (condition_ID < 0 || condition_ID >= conditions.size()) return false;

    godot::Ref<Incident_Condition> condition = conditions[condition_ID];
    if (_can_record_undo()) {
        undo_redo_->create_action("Remove Incident Condition");
        undo_redo_->add_do_method(callable_mp(this, &Incident::_remove_condition_at_direct).bind(condition_ID));
        undo_redo_->add_undo_method(callable_mp(this, &Incident::_insert_condition_at_direct).bind(condition_ID, condition));
        undo_redo_->commit_action();
    } else {
        _remove_condition_at_direct(condition_ID);
    }
    return true;
}

void Incident::_connect_to_condition(const godot::Ref<Incident_Condition> &condition) {
    godot::Ref<Narrative_Condition> narcon = condition;
    if (narcon.is_valid()) {
        if (!narcon->is_connected("subject_changed", godot::Callable(this, "_narcon_subject_change"))) {
            narcon->connect("subject_changed", godot::Callable(this, "_narcon_subject_change"));
        }
        if (!narcon->is_connected("object_changed", godot::Callable(this, "_narcon_object_change"))) {
            narcon->connect("object_changed", godot::Callable(this, "_narcon_object_change"));
        }
        if (!narcon->is_connected("condition_changed", godot::Callable(this, "_narcon_condition_change"))) {
            narcon->connect("condition_changed", godot::Callable(this, "_narcon_condition_change"));
        }
    }
}

void Incident::_disconnect_from_condition(const godot::Ref<Incident_Condition> &condition) {
    godot::Ref<Narrative_Condition> narcon = condition;
    if (narcon.is_valid()) {
        if (narcon->is_connected("subject_changed", godot::Callable(this, "_narcon_subject_change"))) {
            narcon->disconnect("subject_changed", godot::Callable(this, "_narcon_subject_change"));
        }
        if (narcon->is_connected("object_changed", godot::Callable(this, "_narcon_object_change"))) {
            narcon->disconnect("object_changed", godot::Callable(this, "_narcon_object_change"));
        }
        if (narcon->is_connected("condition_changed", godot::Callable(this, "_narcon_condition_change"))) {
            narcon->disconnect("condition_changed", godot::Callable(this, "_narcon_condition_change"));
        }
    }
}

void Incident::_narcon_subject_change(const godot::Ref<Narrative_Condition> &narcon) {
    (void)narcon;
}

void Incident::_narcon_object_change(const godot::Ref<Narrative_Condition> &narcon) {
    (void)narcon;
}

void Incident::_narcon_condition_change(const godot::Ref<Narrative_Condition> &narcon) {
    (void)narcon;
}

godot::TypedArray<godot::Dictionary> Incident::get_narrative_condition_descriptors(Narreme *p_narreme) const {
    godot::TypedArray<godot::Dictionary> out;

    auto make_tags = [](std::initializer_list<const char *> p_list) {
        godot::PackedStringArray tags;
        for (const char *t : p_list) {
            tags.append(godot::String(t));
        }
        return tags;
    };

    if (p_narreme) {
        const godot::String obj_cls = p_narreme->get_class_name_str();
        out.append(make_condition_descriptor(
            0, "incident.has_condition_subject", "has condition subject",
            "Event_participation", "Event", "Agent",
            make_tags({"event", "participant", "subject"}), true, obj_cls
        ));
        out.append(make_condition_descriptor(
            1, "incident.has_condition_object", "has condition object",
            "Event_participation", "Event", "Patient",
            make_tags({"event", "participant", "object"}), true, obj_cls
        ));
    } else {
        out.append(make_condition_descriptor(
            0, "incident.has_not_happened", "has not happened",
            "Event_occurrence", "Event", "",
            make_tags({"event", "temporal", "pending"}), false
        ));
        out.append(make_condition_descriptor(
            1, "incident.has_happened", "has happened",
            "Event_occurrence", "Event", "",
            make_tags({"event", "temporal", "occurred"}), false
        ));
        out.append(make_condition_descriptor(
            2, "incident.cannot_happen", "cannot happen",
            "Event_occurrence", "Event", "",
            make_tags({"event", "temporal", "impossible"}), false
        ));
    }

    return out;
}

godot::Array Incident::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Incident::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        switch (p_condition_id) {
            case 0:
                for (int i = 0; i < conditions.size(); i++) {
                    godot::Ref<Narrative_Condition> narcon = conditions[i];
                    if (narcon.is_valid() && narcon->get_subject().ptr() == p_conditional_narreme) {
                        return NarremeConditionStatus::MET;
                    }
                }
                return NarremeConditionStatus::NOT_MET;
            case 1:
                for (int i = 0; i < conditions.size(); i++) {
                    godot::Ref<Narrative_Condition> narcon = conditions[i];
                    if (narcon.is_valid() && narcon->get_object().ptr() == p_conditional_narreme) {
                        return NarremeConditionStatus::MET;
                    }
                }
                return NarremeConditionStatus::NOT_MET;
        }
    } else {
        switch (p_condition_id) {
            case 0: return (status == IncidentStatus::HAS_NOT_HAPPENED) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1: return (status == IncidentStatus::HAS_HAPPENED) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2: return (status == IncidentStatus::CANNOT_HAPPEN) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Incident::get_class_name_str() const {
    return "Incident";
}

} // namespace ideam::godot_ext