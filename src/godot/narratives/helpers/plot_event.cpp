#include "plot_event.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "../narremes/incident.h"

namespace ideam::godot_ext {

void Plot_Event::_bind_methods() {
    using namespace godot;

    // Undo / Redo Property
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &Plot_Event::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &Plot_Event::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_title", "title"), &Plot_Event::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &Plot_Event::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_incident", "incident"), &Plot_Event::set_incident);
    ClassDB::bind_method(D_METHOD("get_incident"), &Plot_Event::get_incident);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "incident", PROPERTY_HINT_RESOURCE_TYPE, "Incident"), "set_incident", "get_incident");

    ClassDB::bind_method(D_METHOD("set_requirement", "requirement"), &Plot_Event::set_requirement);
    ClassDB::bind_method(D_METHOD("get_requirement"), &Plot_Event::get_requirement);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "requirement"), "set_requirement", "get_requirement");

    ClassDB::bind_method(
        D_METHOD("commit_edit", "title", "incident", "requirement", "action_name"),
        &Plot_Event::commit_edit,
        DEFVAL(String("Edit Plot Event"))
    );

    ClassDB::bind_method(D_METHOD("check_requirement"), &Plot_Event::check_requirement);
    ClassDB::bind_method(D_METHOD("_cannot_happen"), &Plot_Event::_cannot_happen);
    ClassDB::bind_method(D_METHOD("_happened"), &Plot_Event::_happened);

    ADD_SIGNAL(MethodInfo("status_changed"));
    ADD_SIGNAL(MethodInfo("incident_changed", PropertyInfo(Variant::OBJECT, "plot_event", PROPERTY_HINT_RESOURCE_TYPE, "Plot_Event")));

    BIND_ENUM_CONSTANT(INCIDENT_REQ_HAS_NOT_HAPPENED);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_HAS_HAPPENED);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_CANNOT_HAPPEN);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_INSTANT_SUCCESS);
    BIND_ENUM_CONSTANT(INCIDENT_REQ_INSTANT_FAILURE);
}

Plot_Event::Plot_Event() = default;
Plot_Event::~Plot_Event() = default;

void Plot_Event::initialize() {
    if (_is_ready) return;

    _is_ready = true;
    _connect_to_incident();
}

void Plot_Event::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
}

void Plot_Event::set_title(const godot::String &p_title) {
    if (title == p_title) return;
    title = p_title;
    emit_changed();
}
godot::String Plot_Event::get_title() const { return title; }

void Plot_Event::set_incident(const godot::Ref<Incident> &p_incident) {
    if (incident == p_incident) return;
    if (_is_ready) {
        _disconnect_from_incident();
    }
    incident = p_incident;
    if (_is_ready) {
        _connect_to_incident();
        emit_signal("incident_changed", this);
    }
    emit_changed();
}
godot::Ref<Incident> Plot_Event::get_incident() const { return incident; }

void Plot_Event::set_requirement(IncidentRequirement p_requirement) {
    if (requirement == p_requirement) return;
    requirement = p_requirement;
    emit_signal("status_changed");
    emit_changed();
}
IncidentRequirement Plot_Event::get_requirement() const { return requirement; }

void Plot_Event::_apply_edit_direct(
    const godot::String &p_title,
    const godot::Ref<Incident> &p_incident,
    int p_requirement
) {
    const bool inc_changed = (incident != p_incident);
    const bool req_changed = (static_cast<int>(requirement) != p_requirement);
    if (_is_ready && inc_changed) {
        _disconnect_from_incident();
    }
    title       = p_title;
    incident    = p_incident;
    requirement = static_cast<IncidentRequirement>(p_requirement);
    if (_is_ready && inc_changed) {
        _connect_to_incident();
        emit_signal("incident_changed", this);
    }
    if (req_changed) {
        emit_signal("status_changed");
    }
    emit_changed();
}

void Plot_Event::commit_edit(
    const godot::String &p_title,
    const godot::Ref<Incident> &p_incident,
    IncidentRequirement p_requirement,
    const godot::String &p_action_name
) {
    if (title == p_title && incident == p_incident && requirement == p_requirement) {
        return;
    }
    if (_can_record_undo()) {
        const godot::String old_title        = title;
        const godot::Ref<Incident> old_inc   = incident;
        const int old_req                    = static_cast<int>(requirement);
        const int new_req                    = static_cast<int>(p_requirement);

        undo_redo_->create_action(p_action_name.is_empty() ? godot::String("Edit Plot Event") : p_action_name);
        undo_redo_->add_do_method(callable_mp(this, &Plot_Event::_apply_edit_direct).bind(p_title, p_incident, new_req));
        undo_redo_->add_undo_method(callable_mp(this, &Plot_Event::_apply_edit_direct).bind(old_title, old_inc, old_req));
        undo_redo_->commit_action();
    } else {
        _apply_edit_direct(p_title, p_incident, static_cast<int>(p_requirement));
    }
}

bool Plot_Event::check_requirement() {
    if (!incident.is_valid()) {
        godot::UtilityFunctions::print("No Incident, no way!");
        return false;
    }

    IncidentStatus inc_status = incident->get_status();

    switch (requirement) {
        case INCIDENT_REQ_HAS_NOT_HAPPENED:
            return (inc_status == IncidentStatus::HAS_NOT_HAPPENED);

        case INCIDENT_REQ_HAS_HAPPENED:
            return (inc_status == IncidentStatus::HAS_HAPPENED);

        case INCIDENT_REQ_CANNOT_HAPPEN:
            return (inc_status == IncidentStatus::CANNOT_HAPPEN);

        case INCIDENT_REQ_INSTANT_SUCCESS:
            if (!_instant && inc_status == IncidentStatus::HAS_HAPPENED) {
                _instant = true;
                return true;
            }
            break;

        case INCIDENT_REQ_INSTANT_FAILURE:
            if (!_instant && inc_status == IncidentStatus::CANNOT_HAPPEN) {
                _instant = true;
                return true;
            }
            break;
    }

    return false;
}

void Plot_Event::_connect_to_incident() {
    if (!incident.is_valid()) return;

    if (!incident->is_connected("cannot_happen", godot::Callable(this, "_cannot_happen"))) {
        incident->connect("cannot_happen", godot::Callable(this, "_cannot_happen"));
    }

    if (!incident->is_connected("happened", godot::Callable(this, "_happened"))) {
        incident->connect("happened", godot::Callable(this, "_happened"));
    }
}

void Plot_Event::_disconnect_from_incident() {
    if (!incident.is_valid()) return;

    if (incident->is_connected("cannot_happen", godot::Callable(this, "_cannot_happen"))) {
        incident->disconnect("cannot_happen", godot::Callable(this, "_cannot_happen"));
    }

    if (incident->is_connected("happened", godot::Callable(this, "_happened"))) {
        incident->disconnect("happened", godot::Callable(this, "_happened"));
    }
}

void Plot_Event::_cannot_happen() {
    emit_signal("incident_changed", this);
}

void Plot_Event::_happened() {
    emit_signal("incident_changed", this);
}

} // namespace ideam::godot_ext