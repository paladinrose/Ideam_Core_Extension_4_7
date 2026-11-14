#pragma once

/**
 * sememe_resource.h
 * `SememeResource`, `SemanticTagSetResource`, and `SemanticFrameResource`
 * (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & INDUSTRY-STANDARD NLP ALIGNMENT
 * ============================================================================
 * 1. `SememeResource` (WordNet Synset + Hybrid Symbolic & MRL Binary Embedding):
 *    - Aligns with **WordNet Synsets** (`wordnet_synset_id`, `hypernym_labels`, `holonym_labels`,
 *      `antonym_seme_tags`, `gloss_description`) and **Universal Dependencies v2 (`UPOS`)**
 *      17 part-of-speech tags (`lexical_category`).
 *    - Supports **Matryoshka Representation Learning (MRL)** float embeddings (`mrl_float_embedding`)
 *      from external models (OpenAI `text-embedding-3`, Nomic, BGE, SentenceTransformers),
 *      automatically sign-binarizing them into the 128-bit nuclear MRL slice (`nuclear_seme_words[1..2]`)
 *      and 192-bit peripheral tail (`peripheral_seme_words[0..2]`) while keeping `nuclear_seme_words[0]`
 *      for exact 64-bit symbolic invariants (`CanonicalSemeBit`).
 *
 * 2. `SemanticTagSetResource`:
 *    - Unions multiple `SememeResource`s, explicit symbolic tags, and optional MRL float embeddings
 *      into a single 64-byte `core::SememeDescriptorPOD` / `core::LinguisticProfilePOD`.
 *
 * 3. `SemanticFrameResource` (FrameNet / PropBank Predicate-Argument Structure):
 *    - Models conceptual event frames (e.g., `Attack`, `Closure_Open`, `Communication_Statement`,
 *      `Commerce_Buy`) with typed participant selectional restrictions (`Agent`/`ARG0`,
 *      `Patient`/`ARG1`, `Instrument`/`ARG2`) and compiles into a 32-byte `core::SemanticFramePOD`.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "../../core/linguistics/semantic_lexicon_core.h"
#include "../utilities/ideam_undo_redo.h"

namespace ideam::godot_ext {

class SememeResource : public godot::Resource {
    GDCLASS(SememeResource, godot::Resource)

private:
    int unique_id_          = 0;
    int sememic_form_       = 2; // Primary Denotation
    int lexical_category_   = static_cast<int>(core::UniversalPOS::NOUN);
    int primary_domain_id_  = static_cast<int>(core::SemanticDomain::GENERAL);
    float emotive_valence_  = 0.0f;
    godot::StringName canonical_label_;
    godot::String wordnet_synset_id_; // e.g., "castle.n.01" or "02979883-n"
    godot::String gloss_description_;
    godot::PackedStringArray nuclear_seme_tags_;
    godot::PackedStringArray peripheral_seme_tags_;
    godot::PackedStringArray antonym_seme_tags_;
    godot::PackedStringArray hypernym_labels_;
    godot::PackedStringArray holonym_labels_;
    godot::PackedFloat32Array mrl_float_embedding_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    mutable core::SememeDescriptorPOD cached_descriptor_{};
    mutable bool descriptor_dirty_ = true;

    void _rebuild_descriptor_if_dirty() const;

protected:
    static void _bind_methods();

public:
    SememeResource() = default;
    ~SememeResource() override = default;

    // --- Undo / Redo Injection ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    // --- Core Identity, WordNet Synset & UPOS Properties ---
    void set_unique_id(int p_id);
    [[nodiscard]] int get_unique_id() const noexcept { return unique_id_; }

    void set_canonical_label(const godot::StringName &p_label);
    [[nodiscard]] godot::StringName get_canonical_label() const { return canonical_label_; }

    void set_wordnet_synset_id(const godot::String &p_synset_id);
    [[nodiscard]] godot::String get_wordnet_synset_id() const { return wordnet_synset_id_; }

    void set_sememic_form(int p_form);
    [[nodiscard]] int get_sememic_form() const noexcept { return sememic_form_; }

    void set_lexical_category(int p_category);
    [[nodiscard]] int get_lexical_category() const noexcept { return lexical_category_; }
    [[nodiscard]] godot::String get_upos_tag() const;
    void set_upos_tag(const godot::String &p_upos);

    void set_primary_domain_id(int p_domain);
    [[nodiscard]] int get_primary_domain_id() const noexcept { return primary_domain_id_; }

    void set_emotive_valence(float p_valence);
    [[nodiscard]] float get_emotive_valence() const noexcept { return emotive_valence_; }

    void set_gloss_description(const godot::String &p_gloss);
    [[nodiscard]] godot::String get_gloss_description() const { return gloss_description_; }

    // --- Nuclear, Peripheral, Antonym & WordNet Taxonomic Tags ---
    void set_nuclear_seme_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_nuclear_seme_tags() const { return nuclear_seme_tags_; }

    void set_peripheral_seme_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_peripheral_seme_tags() const { return peripheral_seme_tags_; }

    void set_antonym_seme_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_antonym_seme_tags() const { return antonym_seme_tags_; }

    void set_hypernym_labels(const godot::PackedStringArray &p_labels);
    [[nodiscard]] godot::PackedStringArray get_hypernym_labels() const { return hypernym_labels_; }

    void set_holonym_labels(const godot::PackedStringArray &p_labels);
    [[nodiscard]] godot::PackedStringArray get_holonym_labels() const { return holonym_labels_; }

    // --- Matryoshka (MRL) Float & Binary-Quantized Embedding ---
    void set_mrl_float_embedding(const godot::PackedFloat32Array &p_embedding);
    [[nodiscard]] godot::PackedFloat32Array get_mrl_float_embedding() const { return mrl_float_embedding_; }
    [[nodiscard]] godot::PackedInt64Array get_mrl_binary_embedding_words() const;
    [[nodiscard]] float compute_mrl_hamming_similarity_with(const godot::Ref<SememeResource> &p_other) const;

    void add_nuclear_seme_tag(const godot::String &p_tag);
    void remove_nuclear_seme_tag(const godot::String &p_tag);
    void add_peripheral_seme_tag(const godot::String &p_tag);
    void remove_peripheral_seme_tag(const godot::String &p_tag);

    [[nodiscard]] bool has_nuclear_seme(const godot::StringName &p_tag) const;
    [[nodiscard]] bool has_peripheral_seme(const godot::StringName &p_tag) const;
    [[nodiscard]] bool has_any_seme(const godot::StringName &p_tag) const;

    // --- Compiled 192-Bit Bitmasks & DOD Descriptor ---
    [[nodiscard]] const core::SememeDescriptorPOD &get_compiled_descriptor() const;
    [[nodiscard]] godot::PackedInt64Array get_nuclear_bitmask_words() const;
    [[nodiscard]] godot::PackedInt64Array get_peripheral_bitmask_words() const;
    [[nodiscard]] int get_nuclear_popcount() const;
    [[nodiscard]] int get_peripheral_popcount() const;
    [[nodiscard]] int64_t get_coherence_trait_mask32() const;

    // --- Componential & Hybrid Neuro-Symbolic Affinity Evaluation ---
    [[nodiscard]] float compute_affinity_with(
        const godot::Ref<SememeResource> &p_other,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    ) const;

    [[nodiscard]] float compute_tversky_affinity_with(
        const godot::Ref<SememeResource> &p_other,
        float p_alpha = 0.75f,
        float p_beta = 0.25f
    ) const;

    // --- Reversible Snapshot Mutation API (`IdeamUndoRedo`) ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
    void commit_seme_tags(
        const godot::PackedStringArray &p_nuclear_tags,
        const godot::PackedStringArray &p_peripheral_tags,
        const godot::String &p_action_name = "Modify Sememe Tags"
    );
};

class SemanticTagSetResource : public godot::Resource {
    GDCLASS(SemanticTagSetResource, godot::Resource)

private:
    godot::TypedArray<SememeResource> sememes_;
    godot::PackedStringArray explicit_nuclear_tags_;
    godot::PackedStringArray explicit_peripheral_tags_;
    godot::PackedStringArray excluded_seme_tags_;
    godot::PackedFloat32Array mrl_float_embedding_;
    godot::String source_prompt_;
    float aggregate_valence_ = 0.0f;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    mutable core::SememeDescriptorPOD cached_union_descriptor_{};
    mutable bool union_dirty_ = true;

    void _rebuild_union_descriptor_if_dirty() const;

protected:
    static void _bind_methods();

public:
    SemanticTagSetResource() = default;
    ~SemanticTagSetResource() override = default;

    // --- Undo / Redo Injection ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    // --- Properties ---
    void set_sememes(const godot::TypedArray<SememeResource> &p_sememes);
    [[nodiscard]] godot::TypedArray<SememeResource> get_sememes() const { return sememes_; }

    void set_explicit_nuclear_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_explicit_nuclear_tags() const { return explicit_nuclear_tags_; }

    void set_explicit_peripheral_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_explicit_peripheral_tags() const { return explicit_peripheral_tags_; }

    void set_excluded_seme_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_excluded_seme_tags() const { return excluded_seme_tags_; }

    void set_mrl_float_embedding(const godot::PackedFloat32Array &p_embedding);
    [[nodiscard]] godot::PackedFloat32Array get_mrl_float_embedding() const { return mrl_float_embedding_; }

    void set_source_prompt(const godot::String &p_prompt);
    [[nodiscard]] godot::String get_source_prompt() const { return source_prompt_; }

    void set_aggregate_valence(float p_valence);
    [[nodiscard]] float get_aggregate_valence() const noexcept { return aggregate_valence_; }

    // --- Tag Set Manipulation ---
    void add_sememe(const godot::Ref<SememeResource> &p_sememe);
    void remove_sememe(const godot::Ref<SememeResource> &p_sememe);
    void clear_sememes();

    void add_nuclear_tag(const godot::String &p_tag);
    void add_peripheral_tag(const godot::String &p_tag);
    void add_excluded_tag(const godot::String &p_tag);

    [[nodiscard]] bool has_nuclear_tag(const godot::StringName &p_tag) const;
    [[nodiscard]] bool has_peripheral_tag(const godot::StringName &p_tag) const;
    [[nodiscard]] bool has_any_tag(const godot::StringName &p_tag) const;

    [[nodiscard]] godot::PackedStringArray get_all_nuclear_tags() const;
    [[nodiscard]] godot::PackedStringArray get_all_peripheral_tags() const;

    void merge_from(const godot::Ref<SemanticTagSetResource> &p_other);

    // --- Compiled Bitmasks & DOD Serialization ---
    void mark_dirty() const noexcept { union_dirty_ = true; }
    [[nodiscard]] const core::SememeDescriptorPOD &get_compiled_descriptor() const;
    [[nodiscard]] godot::PackedInt64Array get_nuclear_bitmask_words() const;
    [[nodiscard]] godot::PackedInt64Array get_peripheral_bitmask_words() const;
    void set_raw_bitmask_words(
        const godot::PackedInt64Array &p_nuclear_words,
        const godot::PackedInt64Array &p_peripheral_words
    );
    [[nodiscard]] int get_nuclear_popcount() const;
    [[nodiscard]] int get_peripheral_popcount() const;
    [[nodiscard]] int64_t get_coherence_trait_mask32() const;

    [[nodiscard]] core::LinguisticProfilePOD to_linguistic_profile_pod(
        uint16_t p_source_channel_mask = core::LING_SOURCE_EXPLICIT_RESOURCE
    ) const;
    [[nodiscard]] godot::PackedByteArray to_profile_pod_bytes(
        int p_source_channel_mask = static_cast<int>(core::LING_SOURCE_EXPLICIT_RESOURCE)
    ) const;
    void load_from_profile_pod_bytes(const godot::PackedByteArray &p_bytes);

    // --- Affinity & Cascade Matching ---
    [[nodiscard]] float compute_affinity_with(
        const godot::Ref<SemanticTagSetResource> &p_other,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    ) const;

    [[nodiscard]] float compute_tversky_affinity_with(
        const godot::Ref<SemanticTagSetResource> &p_candidate,
        float p_alpha = 0.75f,
        float p_beta = 0.25f
    ) const;

    [[nodiscard]] bool matches_required_nuclear_tags(
        const godot::Ref<SemanticTagSetResource> &p_required
    ) const;

    // --- Reversible Snapshot Mutation API (`IdeamUndoRedo`) ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
};

/**
 * SemanticFrameResource
 * FrameNet / PropBank / VerbNet Predicate-Argument Frame Resource (`src/godot/linguistics/`).
 * Models an event or relation schema (`frame_name`, evoking verb/noun `lexical_units`, and
 * selectional restrictions on `Agent`/`ARG0`, `Patient`/`ARG1`, and `Instrument`/`ARG2`).
 */
class SemanticFrameResource : public godot::Resource {
    GDCLASS(SemanticFrameResource, godot::Resource)

private:
    godot::StringName frame_name_;
    godot::StringName parent_frame_name_;
    godot::PackedStringArray evoking_lexical_units_;
    godot::PackedStringArray required_agent_seme_tags_;
    godot::PackedStringArray required_patient_seme_tags_;
    godot::PackedStringArray required_instrument_seme_tags_;
    godot::PackedStringArray consequent_state_seme_tags_;
    float valence_polarity_ = 0.0f;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    SemanticFrameResource() = default;
    ~SemanticFrameResource() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_frame_name(const godot::StringName &p_name) { frame_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::StringName get_frame_name() const { return frame_name_; }

    void set_parent_frame_name(const godot::StringName &p_parent) { parent_frame_name_ = p_parent; emit_changed(); }
    [[nodiscard]] godot::StringName get_parent_frame_name() const { return parent_frame_name_; }

    void set_evoking_lexical_units(const godot::PackedStringArray &p_units) { evoking_lexical_units_ = p_units; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_evoking_lexical_units() const { return evoking_lexical_units_; }

    void set_required_agent_seme_tags(const godot::PackedStringArray &p_tags) { required_agent_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_required_agent_seme_tags() const { return required_agent_seme_tags_; }

    void set_required_patient_seme_tags(const godot::PackedStringArray &p_tags) { required_patient_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_required_patient_seme_tags() const { return required_patient_seme_tags_; }

    void set_required_instrument_seme_tags(const godot::PackedStringArray &p_tags) { required_instrument_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_required_instrument_seme_tags() const { return required_instrument_seme_tags_; }

    void set_consequent_state_seme_tags(const godot::PackedStringArray &p_tags) { consequent_state_seme_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_consequent_state_seme_tags() const { return consequent_state_seme_tags_; }

    void set_valence_polarity(float p_val) { valence_polarity_ = std::clamp(p_val, -1.0f, 1.0f); emit_changed(); }
    [[nodiscard]] float get_valence_polarity() const noexcept { return valence_polarity_; }

    [[nodiscard]] core::SemanticFramePOD get_compiled_frame_pod() const;
    [[nodiscard]] float evaluate_role_fit(
        const godot::Ref<SemanticTagSetResource> &p_agent_tags,
        const godot::Ref<SemanticTagSetResource> &p_patient_tags,
        const godot::Ref<SemanticTagSetResource> &p_instrument_tags = godot::Ref<SemanticTagSetResource>()
    ) const;
};

} // namespace ideam::godot_ext
