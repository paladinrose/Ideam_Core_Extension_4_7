#include "relationship.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include "../narreme.h"

namespace ideam::godot_ext {

void Relationship::_bind_methods() {
    using namespace godot;

    // Undo / Redo Property
    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &Relationship::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &Relationship::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    // Properties
    ClassDB::bind_method(D_METHOD("set_title", "title"), &Relationship::set_title);
    ClassDB::bind_method(D_METHOD("get_title"), &Relationship::get_title);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");

    ClassDB::bind_method(D_METHOD("set_relation", "relation"), &Relationship::set_relation);
    ClassDB::bind_method(D_METHOD("get_relation"), &Relationship::get_relation);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "relation", PROPERTY_HINT_RESOURCE_TYPE, "Narreme"), "set_relation", "get_relation");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "semantic_tags"), &Relationship::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &Relationship::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );
    ClassDB::bind_method(D_METHOD("get_or_create_semantic_tags"), &Relationship::get_or_create_semantic_tags);

    ClassDB::bind_method(
        D_METHOD("commit_edit", "title", "relation", "tags", "action_name"),
        &Relationship::commit_edit,
        DEFVAL(Ref<SemanticTagSetResource>()),
        DEFVAL(String("Edit Relationship"))
    );
}

Relationship::Relationship() = default;
Relationship::~Relationship() = default;

void Relationship::set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    undo_redo_ = p_undo_redo;
    if (semantic_tags_.is_valid()) {
        semantic_tags_->set_undo_redo(p_undo_redo);
    }
}

void Relationship::set_title(const godot::String &p_title) {
    if (title == p_title) return;
    title = p_title;
    emit_changed();
}

godot::String Relationship::get_title() const {
    return title;
}

void Relationship::set_relation(const godot::Ref<Narreme> &p_relation) {
    if (relation == p_relation) return;
    relation = p_relation;
    emit_changed();
}

godot::Ref<Narreme> Relationship::get_relation() const {
    return relation;
}

void Relationship::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    if (semantic_tags_ == p_tags) return;
    semantic_tags_ = p_tags;
    if (semantic_tags_.is_valid() && undo_redo_.is_valid()) {
        semantic_tags_->set_undo_redo(undo_redo_);
    }
    emit_changed();
}

godot::Ref<SemanticTagSetResource> Relationship::get_or_create_semantic_tags() {
    if (!semantic_tags_.is_valid()) {
        semantic_tags_.instantiate();
        if (undo_redo_.is_valid()) {
            semantic_tags_->set_undo_redo(undo_redo_);
        }
        emit_changed();
    }
    return semantic_tags_;
}

void Relationship::_apply_edit_direct(
    const godot::String &p_title,
    const godot::Ref<Narreme> &p_relation,
    const godot::Ref<SemanticTagSetResource> &p_tags
) {
    title          = p_title;
    relation       = p_relation;
    semantic_tags_ = p_tags;
    if (semantic_tags_.is_valid() && undo_redo_.is_valid()) {
        semantic_tags_->set_undo_redo(undo_redo_);
    }
    emit_changed();
}

void Relationship::commit_edit(
    const godot::String &p_title,
    const godot::Ref<Narreme> &p_relation,
    const godot::Ref<SemanticTagSetResource> &p_tags,
    const godot::String &p_action_name
) {
    const godot::Ref<SemanticTagSetResource> target_tags = p_tags.is_valid() ? p_tags : semantic_tags_;
    if (title == p_title && relation == p_relation && semantic_tags_ == target_tags) {
        return;
    }

    if (_can_record_undo()) {
        const godot::String old_title                      = title;
        const godot::Ref<Narreme> old_relation             = relation;
        const godot::Ref<SemanticTagSetResource> old_tags  = semantic_tags_;

        undo_redo_->create_action(p_action_name.is_empty() ? godot::String("Edit Relationship") : p_action_name);
        undo_redo_->add_do_method(callable_mp(this, &Relationship::_apply_edit_direct).bind(p_title, p_relation, target_tags));
        undo_redo_->add_undo_method(callable_mp(this, &Relationship::_apply_edit_direct).bind(old_title, old_relation, old_tags));
        undo_redo_->commit_action();
    } else {
        _apply_edit_direct(p_title, p_relation, target_tags);
    }
}

} // namespace ideam::godot_ext