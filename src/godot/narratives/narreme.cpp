#include "narreme.h"

#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace ideam::godot_ext {

void Narreme::_bind_methods() {
    using namespace godot;

    // Undo / Redo Property (hidden from Inspector, auto-injected by IdeamEditorInspectorPlugin)
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &Narreme::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &Narreme::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    // Core Identity Property
    ClassDB::bind_method(D_METHOD("set_official_name", "official_name"), &Narreme::set_official_name);
    ClassDB::bind_method(D_METHOD("get_official_name"), &Narreme::get_official_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "official_name"), "set_official_name", "get_official_name");

    ClassDB::bind_method(
        D_METHOD("commit_official_name", "name", "action_name"),
        &Narreme::commit_official_name,
        DEFVAL(String("Rename Narreme"))
    );

    // Linguistics / Semantics Property
    ClassDB::bind_method(D_METHOD("set_semantic_tags", "semantic_tags"), &Narreme::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &Narreme::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );
    ClassDB::bind_method(D_METHOD("get_or_create_semantic_tags"), &Narreme::get_or_create_semantic_tags);
    ClassDB::bind_method(
        D_METHOD("commit_semantic_tags", "tags", "action_name"),
        &Narreme::commit_semantic_tags,
        DEFVAL(String("Set Narreme Semantic Tags"))
    );
    ClassDB::bind_method(D_METHOD("compute_semantic_affinity", "other"), &Narreme::compute_semantic_affinity);

    // Help / Tutorials Topic Override
    ClassDB::bind_method(D_METHOD("set_help_topic_id", "topic_id"), &Narreme::set_help_topic_id);
    ClassDB::bind_method(D_METHOD("get_help_topic_id"), &Narreme::get_help_topic_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "help_topic_id"), "set_help_topic_id", "get_help_topic_id");

    // Method Bindings
    ClassDB::bind_method(D_METHOD("initialize"), &Narreme::initialize);
    ClassDB::bind_method(
        D_METHOD("get_narrative_condition_descriptors", "narreme"),
        &Narreme::get_narrative_condition_descriptors,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("describe_narrative_condition", "condition_id", "narreme"),
        &Narreme::describe_narrative_condition,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("find_condition_id_by_key", "canonical_key", "narreme"),
        &Narreme::find_condition_id_by_key,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("get_narrative_conditions", "narreme"),
        &Narreme::get_narrative_conditions,
        DEFVAL(nullptr)
    );
    ClassDB::bind_method(
        D_METHOD("check_narrative_condition", "conditionID", "conditionalNarreme"),
        &Narreme::check_narrative_condition
    );
    ClassDB::bind_method(D_METHOD("score_similarity", "similar"), &Narreme::score_similarity);
    ClassDB::bind_method(D_METHOD("get_class_name_str"), &Narreme::get_class_name_str);

    // Signals
    ADD_SIGNAL(MethodInfo("official_name_changed", PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo(
        "semantic_tags_changed",
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource")
    ));

    // Enum Binding
    BIND_ENUM_CONSTANT(NarremeConditionStatus::CANNOT_MEET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::NOT_MET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::UNKNOWN);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::MET);
    BIND_ENUM_CONSTANT(NarremeConditionStatus::PERMANENT);
}

Narreme::Narreme() = default;
Narreme::~Narreme() = default;

void Narreme::initialize() {
    if (official_name.is_empty()) {
        official_name = get_name();
    }
}

void Narreme::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
    if (semantic_tags_.is_valid()) {
        semantic_tags_->set_undo_redo(p_undo_redo);
    }
}

void Narreme::_set_official_name_direct(const godot::String &p_name) {
    if (official_name == p_name) {
        return;
    }
    official_name = p_name;
    emit_signal("official_name_changed", official_name);
    emit_changed();
}

void Narreme::set_official_name(const godot::String &p_name) {
    _set_official_name_direct(p_name);
}

godot::String Narreme::get_official_name() const {
    return official_name;
}

void Narreme::commit_official_name(const godot::String &p_name, const godot::String &p_action_name) {
    if (official_name == p_name) {
        return;
    }
    if (_can_record_undo()) {
        const godot::String old_name = official_name;
        undo_redo_->create_action(p_action_name.is_empty() ? godot::String("Rename Narreme") : p_action_name);
        undo_redo_->add_do_method(callable_mp(this, &Narreme::_set_official_name_direct).bind(p_name));
        undo_redo_->add_undo_method(callable_mp(this, &Narreme::_set_official_name_direct).bind(old_name));
        undo_redo_->commit_action();
    } else {
        _set_official_name_direct(p_name);
    }
}

void Narreme::_set_semantic_tags_direct(const godot::Ref<SemanticTagSetResource> &p_tags) {
    if (semantic_tags_ == p_tags) {
        return;
    }
    semantic_tags_ = p_tags;
    if (semantic_tags_.is_valid() && undo_redo_.is_valid()) {
        semantic_tags_->set_undo_redo(undo_redo_);
    }
    emit_signal("semantic_tags_changed", semantic_tags_);
    emit_changed();
}

void Narreme::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    _set_semantic_tags_direct(p_tags);
}

godot::Ref<SemanticTagSetResource> Narreme::get_or_create_semantic_tags() {
    if (!semantic_tags_.is_valid()) {
        semantic_tags_.instantiate();
        if (undo_redo_.is_valid()) {
            semantic_tags_->set_undo_redo(undo_redo_);
        }
        emit_signal("semantic_tags_changed", semantic_tags_);
        emit_changed();
    }
    return semantic_tags_;
}

void Narreme::commit_semantic_tags(
    const godot::Ref<SemanticTagSetResource> &p_tags,
    const godot::String &p_action_name
) {
    if (semantic_tags_ == p_tags) {
        return;
    }
    if (_can_record_undo()) {
        const godot::Ref<SemanticTagSetResource> old_tags = semantic_tags_;
        undo_redo_->create_action(
            p_action_name.is_empty() ? godot::String("Set Narreme Semantic Tags") : p_action_name
        );
        undo_redo_->add_do_method(callable_mp(this, &Narreme::_set_semantic_tags_direct).bind(p_tags));
        undo_redo_->add_undo_method(callable_mp(this, &Narreme::_set_semantic_tags_direct).bind(old_tags));
        undo_redo_->commit_action();
    } else {
        _set_semantic_tags_direct(p_tags);
    }
}

float Narreme::compute_semantic_affinity(Narreme *p_other) const {
    if (!p_other || !semantic_tags_.is_valid()) {
        return -1.0f;
    }
    const godot::Ref<SemanticTagSetResource> other_tags = p_other->get_semantic_tags();
    if (!other_tags.is_valid()) {
        return -1.0f;
    }
    return semantic_tags_->compute_affinity_with(other_tags);
}

void Narreme::set_help_topic_id(const godot::StringName &p_topic_id) {
    if (help_topic_id_ == p_topic_id) {
        return;
    }
    help_topic_id_ = p_topic_id;
    emit_changed();
}

godot::StringName Narreme::get_help_topic_id() const {
    if (help_topic_id_ != godot::StringName()) {
        return help_topic_id_;
    }
    return godot::StringName(get_class_name_str());
}

godot::Dictionary Narreme::make_condition_descriptor(
    int p_condition_id,
    const godot::StringName &p_canonical_key,
    const godot::String &p_display_fragment,
    const godot::StringName &p_frame_name,
    const godot::StringName &p_subject_role,
    const godot::StringName &p_object_role,
    const godot::PackedStringArray &p_seme_tags,
    bool p_expects_object,
    const godot::String &p_object_class
) {
    godot::Dictionary d;
    d["condition_id"]     = p_condition_id;
    d["canonical_key"]    = p_canonical_key;
    d["display_fragment"] = p_display_fragment;
    d["frame_name"]       = p_frame_name;
    d["subject_role"]     = p_subject_role;
    d["object_role"]      = p_object_role;
    d["seme_tags"]        = p_seme_tags;
    d["expects_object"]   = p_expects_object;
    d["object_class"]     = p_object_class;
    return d;
}

godot::TypedArray<godot::Dictionary> Narreme::get_narrative_condition_descriptors(Narreme *p_narreme) const {
    (void)p_narreme;
    return godot::TypedArray<godot::Dictionary>();
}

godot::Dictionary Narreme::describe_narrative_condition(int p_condition_id, Narreme *p_narreme) const {
    const godot::TypedArray<godot::Dictionary> descriptors = get_narrative_condition_descriptors(p_narreme);
    for (int i = 0; i < descriptors.size(); ++i) {
        const godot::Dictionary d = descriptors[i];
        if (static_cast<int>(d.get("condition_id", -1)) == p_condition_id) {
            return d;
        }
    }
    return godot::Dictionary();
}

int Narreme::find_condition_id_by_key(const godot::StringName &p_canonical_key, Narreme *p_narreme) const {
    if (p_canonical_key == godot::StringName()) {
        return -1;
    }
    const godot::TypedArray<godot::Dictionary> descriptors = get_narrative_condition_descriptors(p_narreme);
    for (int i = 0; i < descriptors.size(); ++i) {
        const godot::Dictionary d = descriptors[i];
        if (godot::StringName(godot::String(d.get("canonical_key", ""))) == p_canonical_key) {
            return static_cast<int>(d.get("condition_id", -1));
        }
    }
    return -1;
}

godot::Array Narreme::get_narrative_conditions(Narreme *p_narreme) const {
    godot::Array conditions;
    const godot::TypedArray<godot::Dictionary> descriptors = get_narrative_condition_descriptors(p_narreme);
    for (int i = 0; i < descriptors.size(); ++i) {
        const godot::Dictionary d = descriptors[i];
        conditions.append(d.get("display_fragment", godot::String()));
    }
    return conditions;
}

NarremeConditionStatus Narreme::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    (void)p_condition_id;
    (void)p_conditional_narreme;
    return NarremeConditionStatus::UNKNOWN;
}

int Narreme::score_similarity(Narreme *p_similar) const {
    if (!p_similar) {
        return 1;
    }

    if (this->get_class_name_str() != p_similar->get_class_name_str()) {
        return 1;
    }

    // If both narremes carry IdeamLinguistics semantic tags, factor semantic affinity into fit score
    // (0 = high affinity / untagged match, 1 = moderate mismatch, 2 = low affinity).
    const float affinity = compute_semantic_affinity(p_similar);
    if (affinity >= 0.0f) {
        if (affinity >= 0.65f) {
            return 0;
        }
        if (affinity >= 0.30f) {
            return 1;
        }
        return 2;
    }
    return 0;
}

godot::String Narreme::get_class_name_str() const {
    return "Narreme";
}

} // namespace ideam::godot_ext