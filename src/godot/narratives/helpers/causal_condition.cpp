#include "causal_condition.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void Causal_Condition::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_conditions", "conditions"), &Causal_Condition::set_conditions);
    ClassDB::bind_method(D_METHOD("get_conditions"), &Causal_Condition::get_conditions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_ARRAY_TYPE, "Incident_Condition"), "set_conditions", "get_conditions");

    ClassDB::bind_method(D_METHOD("set_causes", "causes"), &Causal_Condition::set_causes);
    ClassDB::bind_method(D_METHOD("get_causes"), &Causal_Condition::get_causes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "causes", PROPERTY_HINT_ARRAY_TYPE, "int"), "set_causes", "get_causes");

    ClassDB::bind_method(D_METHOD("add_condition", "condition", "cause"), &Causal_Condition::add_condition);
    ClassDB::bind_method(D_METHOD("remove_condition_at", "id"), &Causal_Condition::remove_condition_at);

    ADD_SIGNAL(MethodInfo("condition_added", PropertyInfo(Variant::OBJECT, "condition", PROPERTY_HINT_RESOURCE_TYPE, "Incident_Condition"), PropertyInfo(Variant::INT, "cause")));
    ADD_SIGNAL(MethodInfo("condition_removed", PropertyInfo(Variant::INT, "at")));
}

Causal_Condition::Causal_Condition() = default;
Causal_Condition::~Causal_Condition() = default;

void Causal_Condition::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    Incident_Condition::set_undo_redo(p_undo_redo);
    for (int i = 0; i < conditions.size(); ++i) {
        godot::Ref<Incident_Condition> cond = conditions[i];
        if (cond.is_valid() && cond.ptr() != this) {
            cond->set_undo_redo(p_undo_redo);
        }
    }
}

void Causal_Condition::set_conditions(const godot::TypedArray<Incident_Condition> &p_conditions) {
    conditions = p_conditions;
    emit_changed();
}
godot::TypedArray<Incident_Condition> Causal_Condition::get_conditions() const { return conditions; }

void Causal_Condition::set_causes(const godot::TypedArray<int> &p_causes) {
    causes = p_causes;
    emit_changed();
}
godot::TypedArray<int> Causal_Condition::get_causes() const { return causes; }

void Causal_Condition::_insert_condition_at_direct(int p_index, const godot::Ref<Incident_Condition> &p_condition, int p_cause) {
    if (!p_condition.is_valid() || conditions.has(p_condition) || p_condition.ptr() == this) {
        return;
    }
    if (undo_redo_.is_valid()) {
        p_condition->set_undo_redo(undo_redo_);
    }
    const int safe_idx = std::clamp(p_index, 0, static_cast<int>(conditions.size()));
    conditions.insert(safe_idx, p_condition);
    causes.insert(std::clamp(safe_idx, 0, static_cast<int>(causes.size())), p_cause);
    emit_signal("condition_added", p_condition, p_cause);
    emit_changed();
}

void Causal_Condition::_remove_condition_at_direct(int p_index) {
    if (p_index < 0 || p_index >= conditions.size()) {
        return;
    }
    conditions.remove_at(p_index);
    if (p_index < causes.size()) {
        causes.remove_at(p_index);
    }
    emit_signal("condition_removed", p_index);
    emit_changed();
}

void Causal_Condition::add_condition(const godot::Ref<Incident_Condition> &condition, NarremeConditionStatus cause) {
    if (!condition.is_valid() || conditions.has(condition) || condition.ptr() == this) return;

    const int target_idx = static_cast<int>(conditions.size());
    const int cause_int  = static_cast<int>(cause);
    if (_can_record_undo()) {
        undo_redo_->create_action("Add Causal Condition");
        undo_redo_->add_do_method(callable_mp(this, &Causal_Condition::_insert_condition_at_direct).bind(target_idx, condition, cause_int));
        undo_redo_->add_undo_method(callable_mp(this, &Causal_Condition::_remove_condition_at_direct).bind(target_idx));
        undo_redo_->commit_action();
    } else {
        _insert_condition_at_direct(target_idx, condition, cause_int);
    }
}

void Causal_Condition::remove_condition_at(int id) {
    if (id < 0 || id >= conditions.size()) return;

    const godot::Ref<Incident_Condition> removed_cond = conditions[id];
    const int removed_cause = (id < causes.size()) ? static_cast<int>(causes[id]) : static_cast<int>(NarremeConditionStatus::MET);

    if (_can_record_undo()) {
        undo_redo_->create_action("Remove Causal Condition");
        undo_redo_->add_do_method(callable_mp(this, &Causal_Condition::_remove_condition_at_direct).bind(id));
        undo_redo_->add_undo_method(callable_mp(this, &Causal_Condition::_insert_condition_at_direct).bind(id, removed_cond, removed_cause));
        undo_redo_->commit_action();
    } else {
        _remove_condition_at_direct(id);
    }
}

NarremeConditionStatus Causal_Condition::evaluate_condition() {
    NarremeConditionStatus new_status = NarremeConditionStatus::MET;

    for (int i = 0; i < conditions.size(); i++) {
        godot::Ref<Incident_Condition> condition = conditions[i];
        if (!condition.is_valid() || condition.ptr() == this) continue;

        NarremeConditionStatus condition_status = condition->get_last_status();
        const int expected_cause = (i < causes.size()) ? static_cast<int>(causes[i]) : static_cast<int>(NarremeConditionStatus::MET);
        if (static_cast<int>(condition_status) != expected_cause) {
            new_status = NarremeConditionStatus::NOT_MET;
        }
    }

    NarremeConditionStatus current_last_status = get_last_status();

    switch (new_status) {
        case NarremeConditionStatus::MET:
            if (current_last_status == NarremeConditionStatus::NOT_MET) emit_signal("met");
            break;
        case NarremeConditionStatus::NOT_MET:
            if (current_last_status == NarremeConditionStatus::MET) emit_signal("unmet");
            break;
        case NarremeConditionStatus::CANNOT_MEET:
            if (current_last_status == NarremeConditionStatus::NOT_MET || current_last_status == NarremeConditionStatus::MET) {
                emit_signal("cannot_meet");
            }
            break;
        default:
            break;
    }

    set_last_status(new_status);
    return new_status;
}

} // namespace ideam::godot_ext