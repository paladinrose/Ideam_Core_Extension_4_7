#pragma once

#include "incident_condition.h"
#include "../../linguistics/sememe_resource.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

// Forward Declaration
class Narreme;

class Narrative_Condition : public Incident_Condition {
    GDCLASS(Narrative_Condition, Incident_Condition)

private:
    // DOD NOTE: Subject and Object pointers create severe spatial dispersion.
    // In DOD, conditions become flat bitsets evaluated over contiguous SoA buffers.
    godot::Ref<Narreme> _subject;
    godot::Ref<Narreme> _object;
    int _condition = 0;

    bool subject_is_symbolic = false;
    bool object_is_symbolic  = false;

    // Language-agnostic semantic predicate anchor & optional FrameNet frame
    godot::StringName semantic_predicate_key_;
    godot::Ref<SemanticFrameResource> semantic_frame_;

    void _apply_configuration_direct(
        const godot::Ref<Narreme> &p_subject,
        int p_condition,
        const godot::Ref<Narreme> &p_object,
        bool p_subject_symbolic,
        bool p_object_symbolic
    );

protected:
    static void _bind_methods();

public:
    Narrative_Condition();
    ~Narrative_Condition() override;

    virtual void initialize() override;
    virtual void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) override;

    void set_subject(const godot::Ref<Narreme> &p_subject);
    [[nodiscard]] godot::Ref<Narreme> get_subject() const;

    void set_object(const godot::Ref<Narreme> &p_object);
    [[nodiscard]] godot::Ref<Narreme> get_object() const;

    void set_condition(int p_condition);
    [[nodiscard]] int get_condition() const;

    void set_subject_is_symbolic(bool p_val);
    [[nodiscard]] bool get_subject_is_symbolic() const;

    void set_object_is_symbolic(bool p_val);
    [[nodiscard]] bool get_object_is_symbolic() const;

    // --- Linguistics / Semantic Predicate & FrameNet Overlap ---
    void set_semantic_predicate_key(const godot::StringName &p_key);
    [[nodiscard]] godot::StringName get_semantic_predicate_key() const { return semantic_predicate_key_; }

    void set_semantic_frame(const godot::Ref<SemanticFrameResource> &p_frame);
    [[nodiscard]] godot::Ref<SemanticFrameResource> get_semantic_frame() const { return semantic_frame_; }

    void sync_semantic_predicate_key();
    bool set_condition_by_key(const godot::StringName &p_canonical_key);
    [[nodiscard]] godot::Dictionary get_semantic_descriptor() const;
    [[nodiscard]] godot::String format_sentence() const;

    // --- Reversible Atomic Configuration (`IdeamUndoRedo`, used by NarrativeConditionField) ---
    void commit_configuration(
        const godot::Ref<Narreme> &p_subject,
        int p_condition,
        const godot::Ref<Narreme> &p_object,
        bool p_subject_symbolic = false,
        bool p_object_symbolic = false,
        const godot::String &p_action_name = "Configure Narrative Condition"
    );

    virtual NarremeConditionStatus evaluate_condition() override;
};

} // namespace ideam::godot_ext