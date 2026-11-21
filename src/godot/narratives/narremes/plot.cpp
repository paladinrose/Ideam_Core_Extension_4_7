#include "plot.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include "../helpers/plot_event.h"
#include "incident.h"

#include <algorithm>

namespace ideam::godot_ext {

void Plot::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_status", "status"), &Plot::set_status);
    ClassDB::bind_method(D_METHOD("get_status"), &Plot::get_status);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "status"), "set_status", "get_status");

    ClassDB::bind_method(D_METHOD("set_plot_events", "plot_events"), &Plot::set_plot_events);
    ClassDB::bind_method(D_METHOD("get_plot_events"), &Plot::get_plot_events);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "plot_events", PROPERTY_HINT_ARRAY_TYPE, "Plot_Event"), "set_plot_events", "get_plot_events");

    ClassDB::bind_method(D_METHOD("evaluate_plot_status"), &Plot::evaluate_plot_status);
    ClassDB::bind_method(D_METHOD("gather_plot_events"), &Plot::gather_plot_events);
    ClassDB::bind_method(D_METHOD("add_plot_event", "new_plot_event"), &Plot::add_plot_event);
    ClassDB::bind_method(D_METHOD("get_plot_event_id", "plot_event"), &Plot::get_plot_event_id);
    ClassDB::bind_method(D_METHOD("has_plot_event", "plot_event"), &Plot::has_plot_event);
    ClassDB::bind_method(D_METHOD("remove_plot_event", "plot_event"), &Plot::remove_plot_event);
    ClassDB::bind_method(D_METHOD("remove_plot_event_at", "plot_event_ID"), &Plot::remove_plot_event_at);

    ClassDB::bind_method(D_METHOD("_plot_event_change", "plot_event"), &Plot::_plot_event_change);
    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Plot::get_class_name_str);

    ADD_SIGNAL(MethodInfo("begun"));
    ADD_SIGNAL(MethodInfo("completed"));
    ADD_SIGNAL(MethodInfo("cannot_complete"));
    ADD_SIGNAL(MethodInfo("failed"));
    ADD_SIGNAL(MethodInfo("plot_event_added"));
    ADD_SIGNAL(MethodInfo("plot_event_removed"));

    BIND_ENUM_CONSTANT(PLOT_NOT_BEGUN);
    BIND_ENUM_CONSTANT(PLOT_IN_PROGRESS);
    BIND_ENUM_CONSTANT(PLOT_COMPLETE);
    BIND_ENUM_CONSTANT(PLOT_CANNOT_COMPLETE);
    BIND_ENUM_CONSTANT(PLOT_FAILED);
}

Plot::Plot() = default;
Plot::~Plot() = default;

void Plot::initialize() {
    Narreme::initialize();
    gather_plot_events();
}

void Plot::set_status(PlotStatus p_status) {
    status = p_status;
    emit_changed();
}
PlotStatus Plot::get_status() const { return status; }

void Plot::set_plot_events(const godot::TypedArray<Plot_Event> &p_events) {
    plot_events = p_events;
    emit_changed();
}
godot::TypedArray<Plot_Event> Plot::get_plot_events() const { return plot_events; }

void Plot::_insert_plot_event_at_direct(int p_index, const godot::Ref<Plot_Event> &p_event) {
    if (!p_event.is_valid() || plot_events.has(p_event)) {
        return;
    }
    if (undo_redo_.is_valid()) {
        p_event->set_undo_redo(undo_redo_);
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(plot_events.size()));
    plot_events.insert(safe_idx, p_event);
    _connect_to_plot_event(p_event);
    emit_signal("plot_event_added");
    emit_changed();
}

void Plot::_remove_plot_event_at_direct(int p_index) {
    if (p_index < 0 || p_index >= plot_events.size()) {
        return;
    }
    godot::Ref<Plot_Event> plot_event = plot_events[p_index];
    _disconnect_from_plot_event(plot_event);
    plot_events.remove_at(p_index);
    emit_signal("plot_event_removed");
    emit_changed();
}

PlotStatus Plot::evaluate_plot_status() {
    if (status == PLOT_COMPLETE || status == PLOT_CANNOT_COMPLETE || status == PLOT_FAILED) {
        return status;
    }

    PlotStatus s     = status;
    bool allRequired = true;
    bool shouldBreak = false;

    for (int i = 0; i < plot_events.size(); i++) {
        godot::Ref<Plot_Event> plot_event = plot_events[i];
        if (!plot_event.is_valid()) continue;

        shouldBreak = false;
        godot::Ref<Incident> incident = plot_event->get_incident();

        switch (plot_event->get_requirement()) {
            case INCIDENT_REQ_INSTANT_FAILURE:
                if (plot_event->check_requirement()) {
                    s = PLOT_FAILED;
                    emit_signal("failed");
                    shouldBreak = true;
                }
                break;

            case INCIDENT_REQ_INSTANT_SUCCESS:
                if (plot_event->check_requirement()) {
                    s = PLOT_COMPLETE;
                    emit_signal("completed");
                    shouldBreak = true;
                }
                break;

            case INCIDENT_REQ_HAS_HAPPENED:
                if (plot_event->check_requirement()) {
                    if (s != PLOT_CANNOT_COMPLETE) {
                        s = PLOT_IN_PROGRESS;
                    }
                } else if (incident.is_valid() && incident->get_status() == IncidentStatus::CANNOT_HAPPEN) {
                    s = PLOT_CANNOT_COMPLETE;
                    emit_signal("cannot_complete");
                    allRequired = false;
                } else {
                    allRequired = false;
                }
                break;

            case INCIDENT_REQ_CANNOT_HAPPEN:
                if (plot_event->check_requirement()) {
                    if (s != PLOT_CANNOT_COMPLETE) {
                        s = PLOT_IN_PROGRESS;
                    }
                } else if (incident.is_valid() && incident->get_status() == IncidentStatus::HAS_HAPPENED) {
                    s = PLOT_CANNOT_COMPLETE;
                    emit_signal("cannot_complete");
                    allRequired = false;
                } else {
                    allRequired = false;
                }
                break;

            case INCIDENT_REQ_HAS_NOT_HAPPENED:
                break;
        }

        if (shouldBreak) break;
    }

    if (s == PLOT_IN_PROGRESS) {
        if (status == PLOT_NOT_BEGUN) {
            status = PLOT_IN_PROGRESS;
            emit_signal("begun");
        }

        if (!shouldBreak && allRequired) {
            status = PLOT_COMPLETE;
            emit_signal("completed");
            return status;
        }
    }

    status = s;
    return status;
}

void Plot::gather_plot_events() {
    for (int i = 0; i < plot_events.size(); i++) {
        godot::Ref<Plot_Event> plot_event = plot_events[i];
        if (plot_event.is_valid()) {
            _connect_to_plot_event(plot_event);
        }
    }
}

int Plot::add_plot_event(const godot::Ref<Plot_Event> &new_plot_event) {
    if (!new_plot_event.is_valid()) return -1;

    int id = get_plot_event_id(new_plot_event);
    if (id < 0) {
        const int target_idx = static_cast<int>(plot_events.size());
        if (_can_record_undo()) {
            undo_redo_->create_action("Add Plot Event");
            undo_redo_->add_do_method(callable_mp(this, &Plot::_insert_plot_event_at_direct).bind(target_idx, new_plot_event));
            undo_redo_->add_undo_method(callable_mp(this, &Plot::_remove_plot_event_at_direct).bind(target_idx));
            undo_redo_->commit_action();
        } else {
            _insert_plot_event_at_direct(target_idx, new_plot_event);
        }
        id = get_plot_event_id(new_plot_event);
    }

    return id;
}

int Plot::get_plot_event_id(const godot::Ref<Plot_Event> &plot_event) const {
    return plot_events.find(plot_event);
}

bool Plot::has_plot_event(const godot::Ref<Plot_Event> &plot_event) const {
    return plot_events.has(plot_event);
}

bool Plot::remove_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    return remove_plot_event_at(get_plot_event_id(plot_event));
}

bool Plot::remove_plot_event_at(int plot_event_ID) {
    if (plot_event_ID < 0 || plot_event_ID >= plot_events.size()) {
        return false;
    }

    godot::Ref<Plot_Event> plot_event = plot_events[plot_event_ID];
    if (_can_record_undo()) {
        undo_redo_->create_action("Remove Plot Event");
        undo_redo_->add_do_method(callable_mp(this, &Plot::_remove_plot_event_at_direct).bind(plot_event_ID));
        undo_redo_->add_undo_method(callable_mp(this, &Plot::_insert_plot_event_at_direct).bind(plot_event_ID, plot_event));
        undo_redo_->commit_action();
    } else {
        _remove_plot_event_at_direct(plot_event_ID);
    }

    return true;
}

void Plot::_connect_to_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    if (!plot_event.is_valid()) return;
    if (!plot_event->is_connected("incident_changed", godot::Callable(this, "_plot_event_change"))) {
        plot_event->connect("incident_changed", godot::Callable(this, "_plot_event_change"));
    }
}

void Plot::_disconnect_from_plot_event(const godot::Ref<Plot_Event> &plot_event) {
    if (!plot_event.is_valid()) return;
    if (plot_event->is_connected("incident_changed", godot::Callable(this, "_plot_event_change"))) {
        plot_event->disconnect("incident_changed", godot::Callable(this, "_plot_event_change"));
    }
}

void Plot::_plot_event_change(const godot::Ref<Plot_Event> &plot_event) {
    (void)plot_event;
    evaluate_plot_status();
}

godot::TypedArray<godot::Dictionary> Plot::get_narrative_condition_descriptors(Narreme *p_narreme) const {
    godot::TypedArray<godot::Dictionary> out;

    auto make_tags = [](std::initializer_list<const char *> p_list) {
        godot::PackedStringArray tags;
        for (const char *t : p_list) {
            tags.append(godot::String(t));
        }
        return tags;
    };

    if (p_narreme) {
        const godot::String class_name = p_narreme->get_class_name_str();
        if (class_name == "Incident") {
            out.append(make_condition_descriptor(
                0, "plot.has_incident", "has plot event incident",
                "Inclusion", "Total", "Part",
                make_tags({"plot", "event", "composition"}), true, "Incident"
            ));
        } else if (class_name == "Plot") {
            out.append(make_condition_descriptor(
                0, "plot.is_same", "is",
                "Identity", "Entity", "Compare_entity",
                make_tags({"identity", "plot"}), true, "Plot"
            ));
        }
    } else {
        out.append(make_condition_descriptor(
            0, "plot.not_begun", "not begun",
            "Process_status", "Process", "",
            make_tags({"plot", "status", "pending"}), false
        ));
        out.append(make_condition_descriptor(
            1, "plot.in_progress", "in progress",
            "Process_status", "Process", "",
            make_tags({"plot", "status", "active"}), false
        ));
        out.append(make_condition_descriptor(
            2, "plot.complete", "complete",
            "Process_status", "Process", "",
            make_tags({"plot", "status", "completed"}), false
        ));
        out.append(make_condition_descriptor(
            3, "plot.cannot_complete", "cannot complete",
            "Process_status", "Process", "",
            make_tags({"plot", "status", "impossible"}), false
        ));
        out.append(make_condition_descriptor(
            4, "plot.failed", "failed",
            "Process_status", "Process", "",
            make_tags({"plot", "status", "failed"}), false
        ));
    }

    return out;
}

godot::Array Plot::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Plot::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        const godot::String class_name = p_conditional_narreme->get_class_name_str();

        if (class_name == "Incident") {
            switch (p_condition_id) {
                case 0:
                    for (int i = 0; i < plot_events.size(); i++) {
                        godot::Ref<Plot_Event> plot_event = plot_events[i];
                        if (plot_event.is_valid() && plot_event->get_incident().ptr() == p_conditional_narreme) {
                            return NarremeConditionStatus::MET;
                        }
                    }
                    return NarremeConditionStatus::NOT_MET;
            }
        } else if (class_name == "Plot") {
            switch (p_condition_id) {
                case 0:
                    return (this == p_conditional_narreme) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        if (static_cast<int>(status) == p_condition_id) {
            return NarremeConditionStatus::MET;
        } else {
            return NarremeConditionStatus::NOT_MET;
        }
    }

    return NarremeConditionStatus::UNKNOWN;
}

godot::String Plot::get_class_name_str() const {
    return "Plot";
}

} // namespace ideam::godot_ext