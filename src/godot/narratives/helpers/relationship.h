#pragma once

#include "../narreme.h"
#include "../../linguistics/sememe_resource.h"
#include "../../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class Relationship : public godot::Resource {
    GDCLASS(Relationship, godot::Resource)

private:
    // DOD NOTE: While RefCounted is better than Node, instantiating an object
    // for every relationship still creates Array of Structures (AoS) heap fragmentation.
    // In IDEAM Core, we will use a BridgeView to link a parent entity
    // to a contiguous block of child relational IDs and metadata, eliminating
    // this class entirely.
    godot::String title;
    godot::Ref<Narreme> relation;
    godot::Ref<SemanticTagSetResource> semantic_tags_;
    godot::Ref<IdeamUndoRedo> undo_redo_;
    bool undo_redo_suppressed_ = false;

    [[nodiscard]] bool _can_record_undo() const noexcept {
        return undo_redo_.is_valid() && !undo_redo_suppressed_;
    }

    void _apply_edit_direct(
        const godot::String &p_title,
        const godot::Ref<Narreme> &p_relation,
        const godot::Ref<SemanticTagSetResource> &p_tags
    );

protected:
    static void _bind_methods();

public:
    Relationship();
    ~Relationship() override;

    // Undo / Redo Injection
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    // Getters / Setters
    void set_title(const godot::String &p_title);
    [[nodiscard]] godot::String get_title() const;

    void set_relation(const godot::Ref<Narreme> &p_relation);
    [[nodiscard]] godot::Ref<Narreme> get_relation() const;

    // Linguistics / Semantic Tagging Overlap (language-independent relationship semantics)
    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }
    godot::Ref<SemanticTagSetResource> get_or_create_semantic_tags();

    // Reversible Edit API (`IdeamUndoRedo`)
    void commit_edit(
        const godot::String &p_title,
        const godot::Ref<Narreme> &p_relation,
        const godot::Ref<SemanticTagSetResource> &p_tags = godot::Ref<SemanticTagSetResource>(),
        const godot::String &p_action_name = "Edit Relationship"
    );
};

} // namespace ideam::godot_ext