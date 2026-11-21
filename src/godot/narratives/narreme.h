#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

// DOD NOTE: Disentangling this enum is a great move. When we map this to
// IDEAM's "Shadow Buffers" (Column Metadata Sequence), this enum will live
// in a parallel Structure of Arrays (SoA) sequence alongside Group Masks.
// By pulling it out of the class, our SIMD bitwise operations
// won't need to know anything about the `Narreme` class itself.
enum NarremeConditionStatus : int8_t {
    CANNOT_MEET = -2,
    NOT_MET = -1,
    UNKNOWN = 0,
    MET = 1,
    PERMANENT = 2
};

/**
 * RAII guard that temporarily suppresses nested `IdeamUndoRedo` recording on a
 * Narreme (or helper resource) while a parent Narreme or multi-object tool
 * orchestrates a single composite Undo/Redo transaction.
 */
struct ScopedNarremeUndoSuppress {
    bool &flag_ref;
    bool previous_value;

    explicit ScopedNarremeUndoSuppress(bool &p_flag) noexcept
        : flag_ref(p_flag), previous_value(p_flag) {
        flag_ref = true;
    }

    ~ScopedNarremeUndoSuppress() noexcept {
        flag_ref = previous_value;
    }

    ScopedNarremeUndoSuppress(const ScopedNarremeUndoSuppress &) = delete;
    ScopedNarremeUndoSuppress &operator=(const ScopedNarremeUndoSuppress &) = delete;
};

class Narreme : public godot::Resource {
    GDCLASS(Narreme, godot::Resource)

protected:
    static void _bind_methods();

    // Foundational Undo/Redo & Semantic/Tutorials hooks shared by all Narremes
    godot::Ref<IdeamUndoRedo> undo_redo_;
    bool undo_redo_suppressed_ = false;
    godot::Ref<SemanticTagSetResource> semantic_tags_;
    godot::StringName help_topic_id_;

    [[nodiscard]] bool _can_record_undo() const noexcept {
        return undo_redo_.is_valid() && !undo_redo_suppressed_;
    }

    void _set_official_name_direct(const godot::String &p_name);
    void _set_semantic_tags_direct(const godot::Ref<SemanticTagSetResource> &p_tags);

public:
    Narreme();
    ~Narreme() override;

    // GODOT NOTE: Resources do not receive `_ready()` callbacks from the SceneTree.
    // `initialize()` replaces `_ready()`.
    virtual void initialize();

    // --- Object-Owned Undo / Redo Integration (`IdeamUndoRedo`) ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_undo_redo_suppressed(bool p_suppressed) noexcept { undo_redo_suppressed_ = p_suppressed; }
    [[nodiscard]] bool is_undo_redo_suppressed() const noexcept { return undo_redo_suppressed_; }

    // --- Identity ---
    void set_official_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_official_name() const;
    void commit_official_name(
        const godot::String &p_name,
        const godot::String &p_action_name = "Rename Narreme"
    );

    // --- Linguistics / Structural Semantics Overlap ---
    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }
    godot::Ref<SemanticTagSetResource> get_or_create_semantic_tags();
    void commit_semantic_tags(
        const godot::Ref<SemanticTagSetResource> &p_tags,
        const godot::String &p_action_name = "Set Narreme Semantic Tags"
    );
    [[nodiscard]] float compute_semantic_affinity(Narreme *p_other) const;

    // --- Tools & Tutorials Help Topic Hook ---
    void set_help_topic_id(const godot::StringName &p_topic_id);
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    // --- Language-Agnostic Narrative Condition Descriptors & Evaluation ---
    // Helper for subclasses to emit standardized, FrameNet-aligned condition metadata.
    static godot::Dictionary make_condition_descriptor(
        int p_condition_id,
        const godot::StringName &p_canonical_key,
        const godot::String &p_display_fragment,
        const godot::StringName &p_frame_name,
        const godot::StringName &p_subject_role,
        const godot::StringName &p_object_role,
        const godot::PackedStringArray &p_seme_tags,
        bool p_expects_object,
        const godot::String &p_object_class = ""
    );

    // Returns structured Dictionaries for each valid condition on this Narreme (optionally paired with `p_narreme`).
    // Keys: condition_id, canonical_key, display_fragment, frame_name, subject_role, object_role,
    //       seme_tags, expects_object, object_class.
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const;
    godot::Dictionary describe_narrative_condition(int p_condition_id, Narreme *p_narreme = nullptr) const;
    int find_condition_id_by_key(const godot::StringName &p_canonical_key, Narreme *p_narreme = nullptr) const;

    // Backward-compatible display string list (derived from `get_narrative_condition_descriptors()`).
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const;
    virtual int score_similarity(Narreme *p_similar) const;
    virtual godot::String get_class_name_str() const;

private:
    godot::String official_name;
};

} // namespace ideam::godot_ext

// Godot macro requires the fully qualified name to register the variant
VARIANT_ENUM_CAST(ideam::godot_ext::NarremeConditionStatus);