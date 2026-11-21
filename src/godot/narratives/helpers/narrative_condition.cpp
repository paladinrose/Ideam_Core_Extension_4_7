#include "narrative_condition.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include "../narreme.h"

namespace ideam::godot_ext {

void Narrative_Condition::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_subject", "subject"), &Narrative_Condition::set_subject);
    ClassDB::bind_method(D_METHOD("get_subject"), &Narrative_Condition::get_subject);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "subject", PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_subject", "get_subject");

    ClassDB::bind_method(D_METHOD("set_object", "object"), &Narrative_Condition::set_object);
    ClassDB::bind_method(D_METHOD("get_object"), &Narrative_Condition::get_object);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "object", PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_object", "get_object");

    ClassDB::bind_method(D_METHOD("set_condition", "condition"), &Narrative_Condition::set_condition);
    ClassDB::bind_method(D_METHOD("get_condition"), &Narrative_Condition::get_condition);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "condition"), "set_condition", "get_condition");

    ClassDB::bind_method(D_METHOD("set_subject_is_symbolic", "subject_is_symbolic"), &Narrative_Condition::set_subject_is_symbolic);
    ClassDB::bind_method(D_METHOD("get_subject_is_symbolic"), &Narrative_Condition::get_subject_is_symbolic);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "subject_is_symbolic"), "set_subject_is_symbolic", "get_subject_is_symbolic");

    ClassDB::bind_method(D_METHOD("set_object_is_symbolic", "object_is_symbolic"), &Narrative_Condition::set_object_is_symbolic);
    ClassDB::bind_method(D_METHOD("get_object_is_symbolic"), &Narrative_Condition::get_object_is_symbolic);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "object_is_symbolic"), "set_object_is_symbolic", "get_object_is_symbolic");

    // Semantic Predicate & FrameNet Hooks
    ClassDB::bind_method(D_METHOD("set_semantic_predicate_key", "key"), &Narrative_Condition::set_semantic_predicate_key);
    ClassDB::bind_method(D_METHOD("get_semantic_predicate_key"), &Narrative_Condition::get_semantic_predicate_key);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "semantic_predicate_key"), "set_semantic_predicate_key", "get_semantic_predicate_key");

    ClassDB::bind_method(D_METHOD("set_semantic_frame", "semantic_frame"), &Narrative_Condition::set_semantic_frame);
    ClassDB::bind_method(D_METHOD("get_semantic_frame"), &Narrative_Condition::get_semantic_frame);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_frame", PROPERTY_HINT_RESOURCE_TYPE, "SemanticFrameResource"),
        "set_semantic_frame",
        "get_semantic_frame"
    );

    ClassDB::bind_method(D_METHOD("sync_semantic_predicate_key"), &Narrative_Condition::sync_semantic_predicate_key);
    ClassDB::bind_method(D_METHOD("set_condition_by_key", "canonical_key"), &Narrative_Condition::set_condition_by_key);
    ClassDB::bind_method(D_METHOD("get_semantic_descriptor"), &Narrative_Condition::get_semantic_descriptor);
    ClassDB::bind_method(D_METHOD("format_sentence"), &Narrative_Condition::format_sentence);
    ClassDB::bind_method(
        D_METHOD("commit_configuration", "subject", "condition", "object", "subject_symbolic", "object_symbolic", "action_name"),
        &Narrative_Condition::commit_configuration,
        DEFVAL(false),
        DEFVAL(false),
        DEFVAL(String("Configure Narrative Condition"))
    );

    ADD_SIGNAL(MethodInfo("subject_changed", PropertyInfo(Variant::OBJECT, "narcon", PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
    ADD_SIGNAL(MethodInfo("condition_changed", PropertyInfo(Variant::OBJECT, "narcon", PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
    ADD_SIGNAL(MethodInfo("object_changed", PropertyInfo(Variant::OBJECT, "narcon", PROPERTY_HINT_RESOURCE_TYPE, "Narrative_Condition")));
}

Narrative_Condition::Narrative_Condition() = default;
Narrative_Condition::~Narrative_Condition() = default;

void Narrative_Condition::initialize() {
    if (_is_ready) return;
    Incident_Condition::initialize();
    sync_semantic_predicate_key();
}

void Narrative_Condition::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    Incident_Condition::set_undo_redo(p_undo_redo);
    if (semantic_frame_.is_valid()) {
        semantic_frame_->set_undo_redo(p_undo_redo);
    }
}

void Narrative_Condition::set_subject(const godot::Ref<Narreme> &p_subject) {
    if (_subject == p_subject) return;
    _subject = p_subject;
    sync_semantic_predicate_key();
    if (_is_ready) {
        emit_signal("subject_changed", this);
    }
    emit_changed();
}

godot::Ref<Narreme> Narrative_Condition::get_subject() const { return _subject; }

void Narrative_Condition::set_object(const godot::Ref<Narreme> &p_object) {
    if (_object == p_object) return;
    _object = p_object;
    sync_semantic_predicate_key();
    if (_is_ready) {
        emit_signal("object_changed", this);
    }
    emit_changed();
}

godot::Ref<Narreme> Narrative_Condition::get_object() const { return _object; }

void Narrative_Condition::set_condition(int p_condition) {
    if (_condition == p_condition) return;
    _condition = p_condition;
    sync_semantic_predicate_key();
    if (_is_ready) {
        emit_signal("condition_changed", this);
    }
    emit_changed();
}

int Narrative_Condition::get_condition() const { return _condition; }

void Narrative_Condition::set_subject_is_symbolic(bool p_val) {
    if (subject_is_symbolic == p_val) return;
    subject_is_symbolic = p_val;
    emit_changed();
}
bool Narrative_Condition::get_subject_is_symbolic() const { return subject_is_symbolic; }

void Narrative_Condition::set_object_is_symbolic(bool p_val) {
    if (object_is_symbolic == p_val) return;
    object_is_symbolic = p_val;
    emit_changed();
}
bool Narrative_Condition::get_object_is_symbolic() const { return object_is_symbolic; }

void Narrative_Condition::set_semantic_predicate_key(const godot::StringName &p_key) {
    if (semantic_predicate_key_ == p_key) return;
    semantic_predicate_key_ = p_key;
    if (_subject.is_valid() && p_key != godot::StringName()) {
        const int matched_id = _subject->find_condition_id_by_key(p_key, _object.ptr());
        if (matched_id >= 0 && matched_id != _condition) {
            _condition = matched_id;
            if (_is_ready) {
                emit_signal("condition_changed", this);
            }
        }
    }
    emit_changed();
}

void Narrative_Condition::set_semantic_frame(const godot::Ref<SemanticFrameResource> &p_frame) {
    if (semantic_frame_ == p_frame) return;
    semantic_frame_ = p_frame;
    if (semantic_frame_.is_valid() && undo_redo_.is_valid()) {
        semantic_frame_->set_undo_redo(undo_redo_);
    }
    emit_changed();
}

void Narrative_Condition::sync_semantic_predicate_key() {
    if (!_subject.is_valid()) {
        return;
    }
    const godot::Dictionary desc = _subject->describe_narrative_condition(_condition, _object.ptr());
    if (!desc.is_empty()) {
        semantic_predicate_key_ = godot::StringName(godot::String(desc.get("canonical_key", "")));
    }
}

bool Narrative_Condition::set_condition_by_key(const godot::StringName &p_canonical_key) {
    if (!_subject.is_valid()) {
        semantic_predicate_key_ = p_canonical_key;
        return false;
    }
    const int id = _subject->find_condition_id_by_key(p_canonical_key, _object.ptr());
    if (id < 0) {
        return false;
    }
    semantic_predicate_key_ = p_canonical_key;
    set_condition(id);
    return true;
}

godot::Dictionary Narrative_Condition::get_semantic_descriptor() const {
    if (!_subject.is_valid()) {
        return godot::Dictionary();
    }
    return _subject->describe_narrative_condition(_condition, _object.ptr());
}

godot::String Narrative_Condition::format_sentence() const {
    if (!_subject.is_valid()) {
        return godot::String("[No Subject]");
    }

    godot::String subj_name = _subject->get_official_name();
    if (subj_name.is_empty()) {
        subj_name = _subject->get_class_name_str();
    }

    const godot::Dictionary desc = get_semantic_descriptor();
    const godot::String fragment = desc.is_empty()
        ? godot::String("meets condition #") + godot::String::num_int64(_condition)
        : godot::String(desc.get("display_fragment", ""));

    if (_object.is_valid()) {
        godot::String obj_name = _object->get_official_name();
        if (obj_name.is_empty()) {
            obj_name = _object->get_class_name_str();
        }
        return subj_name + " " + fragment + " " + obj_name;
    }
    return subj_name + " " + fragment;
}

void Narrative_Condition::_apply_configuration_direct(
    const godot::Ref<Narreme> &p_subject,
    int p_condition,
    const godot::Ref<Narreme> &p_object,
    bool p_subject_symbolic,
    bool p_object_symbolic
) {
    const bool subj_diff = (_subject != p_subject);
    const bool obj_diff  = (_object != p_object);
    const bool cond_diff = (_condition != p_condition);

    _subject            = p_subject;
    _object             = p_object;
    _condition          = p_condition;
    subject_is_symbolic = p_subject_symbolic;
    object_is_symbolic  = p_object_symbolic;
    sync_semantic_predicate_key();

    if (_is_ready) {
        if (subj_diff) emit_signal("subject_changed", this);
        if (obj_diff) emit_signal("object_changed", this);
        if (cond_diff) emit_signal("condition_changed", this);
    }
    emit_changed();
}

void Narrative_Condition::commit_configuration(
    const godot::Ref<Narreme> &p_subject,
    int p_condition,
    const godot::Ref<Narreme> &p_object,
    bool p_subject_symbolic,
    bool p_object_symbolic,
    const godot::String &p_action_name
) {
    if (_subject == p_subject && _condition == p_condition && _object == p_object &&
        subject_is_symbolic == p_subject_symbolic && object_is_symbolic == p_object_symbolic) {
        return;
    }

    if (_can_record_undo()) {
        const godot::Ref<Narreme> old_subject = _subject;
        const int old_condition               = _condition;
        const godot::Ref<Narreme> old_object  = _object;
        const bool old_subj_sym               = subject_is_symbolic;
        const bool old_obj_sym                = object_is_symbolic;

        undo_redo_->create_action(
            p_action_name.is_empty() ? godot::String("Configure Narrative Condition") : p_action_name
        );
        undo_redo_->add_do_method(callable_mp(this, &Narrative_Condition::_apply_configuration_direct).bind(
            p_subject, p_condition, p_object, p_subject_symbolic, p_object_symbolic
        ));
        undo_redo_->add_undo_method(callable_mp(this, &Narrative_Condition::_apply_configuration_direct).bind(
            old_subject, old_condition, old_object, old_subj_sym, old_obj_sym
        ));
        undo_redo_->commit_action();
    } else {
        _apply_configuration_direct(p_subject, p_condition, p_object, p_subject_symbolic, p_object_symbolic);
    }
}

NarremeConditionStatus Narrative_Condition::evaluate_condition() {
    if (!_subject.is_valid()) return NarremeConditionStatus::CANNOT_MEET;

    NarremeConditionStatus new_status          = _subject->check_narrative_condition(_condition, _object.ptr());
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