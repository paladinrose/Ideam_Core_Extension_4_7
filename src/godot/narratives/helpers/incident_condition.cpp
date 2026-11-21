#include "incident_condition.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace ideam::godot_ext {

void Incident_Condition::_bind_methods() {
    using namespace godot;

    // Undo / Redo Property
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &Incident_Condition::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &Incident_Condition::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_title", "title"), &Incident_Condition::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &Incident_Condition::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(
        D_METHOD("commit_title", "title", "action_name"),
        &Incident_Condition::commit_title,
        DEFVAL(String("Rename Condition"))
    );

    ClassDB::bind_method(D_METHOD("set_last_status", "last_status"), &Incident_Condition::set_last_status);
    ClassDB::bind_method(D_METHOD("get_last_status"), &Incident_Condition::get_last_status);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "last_status"), "set_last_status", "get_last_status");

    ClassDB::bind_method(D_METHOD("evaluate_condition"), &Incident_Condition::evaluate_condition);

    ADD_SIGNAL(MethodInfo("met"));
    ADD_SIGNAL(MethodInfo("unmet"));
    ADD_SIGNAL(MethodInfo("cannot_meet"));
    ADD_SIGNAL(MethodInfo("became_unknown"));
}

Incident_Condition::Incident_Condition() = default;
Incident_Condition::~Incident_Condition() = default;

void Incident_Condition::initialize() {
    if (_is_ready) return;
    _is_ready = true;

    if (title.is_empty()) {
        title = get_class();
    }
}

void Incident_Condition::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
}

void Incident_Condition::_set_title_direct(const godot::String &p_title) {
    if (title == p_title) return;
    title = p_title;
    emit_changed();
}

void Incident_Condition::set_title(const godot::String &p_title) {
    _set_title_direct(p_title);
}

godot::String Incident_Condition::get_title() const { return title; }

void Incident_Condition::commit_title(const godot::String &p_title, const godot::String &p_action_name) {
    if (title == p_title) return;
    if (_can_record_undo()) {
        const godot::String old_title = title;
        undo_redo_->create_action(p_action_name.is_empty() ? godot::String("Rename Condition") : p_action_name);
        undo_redo_->add_do_method(callable_mp(this, &Incident_Condition::_set_title_direct).bind(p_title));
        undo_redo_->add_undo_method(callable_mp(this, &Incident_Condition::_set_title_direct).bind(old_title));
        undo_redo_->commit_action();
    } else {
        _set_title_direct(p_title);
    }
}

void Incident_Condition::set_last_status(NarremeConditionStatus p_status) { last_status = p_status; }
NarremeConditionStatus Incident_Condition::get_last_status() const { return last_status; }

NarremeConditionStatus Incident_Condition::evaluate_condition() {
    return NarremeConditionStatus::UNKNOWN;
}

} // namespace ideam::godot_ext