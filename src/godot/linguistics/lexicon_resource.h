#pragma once

/**
 * lexicon_resource.h
 * `GrammarRuleResource`, `LexiconResource`, and `LanguageAssetResource`
 * (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & INDUSTRY-STANDARD NLP REFACTOR NOTES
 * ============================================================================
 * Godot-first, DOD-backed language & ontology suite aligned with external NLP
 * standards:
 *   - Universal Dependencies v2 (`UPOS` 17-tag part-of-speech enum & `DepRel`).
 *   - CMUdict ARPAbet + PanPhon 16-bit articulatory feature phonology &
 *     Needleman-Wunsch Dynamic Time Warping (`compute_phonetic_distance_between_words`).
 *   - Hugging Face `tokenizer.json` & Google SentencePiece `.vocab` subword BPE
 *     importers (`import_huggingface_tokenizer_json`, `import_sentencepiece_vocab_text`,
 *     `tokenize_bpe_subwords`, `decode_bpe_tokens`).
 *   - WordNet Synset taxonomy (`import_wordnet_synsets_json`, `is_sememe_hypernym_of`)
 *     & Berkeley FrameNet `SemanticFrameResource` thematic role validation.
 *   - Matryoshka Representation Learning (MRL) float-to-1-bit binary quantization
 *     importer (`import_mrl_embeddings_json`).
 *   - GBNF (GGML BNF) context-free grammar exporter (`export_gbnf_grammar`) for
 *     `llama.cpp`, `XGrammar`, and `llguidance` constrained SLM decoding.
 */

#include "sememe_resource.h"
#include "../../core/linguistics/semantic_lexicon_core.h"
#include "../utilities/ideam_undo_redo.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

enum GrammarCascadeLayer : int32_t {
    GRAMMAR_LAYER_BASE_SYNTAX        = 0,
    GRAMMAR_LAYER_MORPHOLOGY         = 1,
    GRAMMAR_LAYER_DIALECT_LECT       = 2,
    GRAMMAR_LAYER_REGISTER_FORMALITY = 3,
    GRAMMAR_LAYER_SLM_CONSTRAINT     = 4,
    GRAMMAR_LAYER_SPEAKER_IDIOLECT   = 5
};

enum GrammarRuleOperation : int32_t {
    GRAMMAR_OP_AFFIX_PREFIX                = 0,
    GRAMMAR_OP_AFFIX_SUFFIX                = 1,
    GRAMMAR_OP_SUBSTITUTE_STEM             = 2,
    GRAMMAR_OP_CONSTRAIN_SLM_NEXT_CATEGORY = 3,
    GRAMMAR_OP_SHIFT_REGISTER_FORMALITY    = 4,
    GRAMMAR_OP_INJECT_PERIPHERAL_SEME      = 5,
    GRAMMAR_OP_EMIT_GBNF_PRODUCTION        = 6
};

class GrammarRuleResource : public godot::Resource {
    GDCLASS(GrammarRuleResource, godot::Resource)

private:
    godot::String rule_name_;
    int priority_                  = 0;
    bool is_important_             = false;
    int cascade_layer_             = GRAMMAR_LAYER_MORPHOLOGY;
    int operation_                 = GRAMMAR_OP_AFFIX_SUFFIX;
    int target_lexical_category_   = static_cast<int>(core::UniversalPOS::UNDEFINED);
    int target_lect_id_            = 0;
    godot::StringName required_seme_tag_;
    godot::StringName forbidden_seme_tag_;
    godot::String match_stem_pattern_;
    godot::String affix_or_replacement_;
    godot::String gbnf_production_pattern_;
    int slm_allowed_category_mask_ = 0x1FFFF; // Bitmask of allowed next UniversalPOS (UPOS 0..16)
    int formality_delta_           = 0;
    godot::TypedArray<GrammarRuleResource> child_rules_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

protected:
    static void _bind_methods();

public:
    GrammarRuleResource() = default;
    ~GrammarRuleResource() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_rule_name(const godot::String &p_name) { rule_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_rule_name() const { return rule_name_; }

    void set_priority(int p_prio) { priority_ = p_prio; emit_changed(); }
    [[nodiscard]] int get_priority() const noexcept { return priority_; }

    void set_is_important(bool p_imp) { is_important_ = p_imp; emit_changed(); }
    [[nodiscard]] bool get_is_important() const noexcept { return is_important_; }

    void set_cascade_layer(int p_layer);
    [[nodiscard]] int get_cascade_layer() const noexcept { return cascade_layer_; }

    void set_operation(int p_op);
    [[nodiscard]] int get_operation() const noexcept { return operation_; }

    void set_target_lexical_category(int p_cat);
    [[nodiscard]] int get_target_lexical_category() const noexcept { return target_lexical_category_; }

    void set_target_lect_id(int p_lect);
    [[nodiscard]] int get_target_lect_id() const noexcept { return target_lect_id_; }

    void set_required_seme_tag(const godot::StringName &p_tag) { required_seme_tag_ = p_tag; emit_changed(); }
    [[nodiscard]] godot::StringName get_required_seme_tag() const { return required_seme_tag_; }

    void set_forbidden_seme_tag(const godot::StringName &p_tag) { forbidden_seme_tag_ = p_tag; emit_changed(); }
    [[nodiscard]] godot::StringName get_forbidden_seme_tag() const { return forbidden_seme_tag_; }

    void set_match_stem_pattern(const godot::String &p_pat) { match_stem_pattern_ = p_pat; emit_changed(); }
    [[nodiscard]] godot::String get_match_stem_pattern() const { return match_stem_pattern_; }

    void set_affix_or_replacement(const godot::String &p_affix) { affix_or_replacement_ = p_affix; emit_changed(); }
    [[nodiscard]] godot::String get_affix_or_replacement() const { return affix_or_replacement_; }

    void set_gbnf_production_pattern(const godot::String &p_gbnf) { gbnf_production_pattern_ = p_gbnf; emit_changed(); }
    [[nodiscard]] godot::String get_gbnf_production_pattern() const { return gbnf_production_pattern_; }

    void set_slm_allowed_category_mask(int p_mask) { slm_allowed_category_mask_ = p_mask; emit_changed(); }
    [[nodiscard]] int get_slm_allowed_category_mask() const noexcept { return slm_allowed_category_mask_; }

    void set_formality_delta(int p_delta) { formality_delta_ = p_delta; emit_changed(); }
    [[nodiscard]] int get_formality_delta() const noexcept { return formality_delta_; }

    void set_child_rules(const godot::TypedArray<GrammarRuleResource> &p_rules) { child_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<GrammarRuleResource> get_child_rules() const { return child_rules_; }

    // --- CSS Specificity & Evaluation ---
    [[nodiscard]] int get_specificity() const;
    [[nodiscard]] bool outweighs(const godot::Ref<GrammarRuleResource> &p_other) const;
    [[nodiscard]] bool matches_context(
        int p_lexical_category,
        int p_lect_id,
        const godot::PackedStringArray &p_active_seme_tags,
        const godot::String &p_surface_word = ""
    ) const;
    [[nodiscard]] godot::String apply_to_surface_word(const godot::String &p_word) const;
    [[nodiscard]] godot::TypedArray<GrammarRuleResource> collect_cascaded_rules() const;

    // --- Undo / Redo Snapshot ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
};

class LexiconResource : public godot::Resource {
    GDCLASS(LexiconResource, godot::Resource)

private:
    godot::String lexicon_name_ = "DefaultLexicon";
    int default_lect_id_        = 0;
    godot::TypedArray<SememeResource> sememe_resources_;
    godot::TypedArray<SemanticFrameResource> semantic_frame_resources_;
    godot::Array morpheme_entries_;
    godot::Array lexeme_entries_;
    godot::Array synset_relation_entries_;
    godot::Array bpe_vocab_entries_;
    godot::Array bpe_merge_entries_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    mutable core::SemanticLexiconCore core_lexicon_;
    mutable bool core_dirty_ = true;

    void _rebuild_core_if_dirty() const;

protected:
    static void _bind_methods();

public:
    LexiconResource();
    ~LexiconResource() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_lexicon_name(const godot::String &p_name) { lexicon_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_lexicon_name() const { return lexicon_name_; }

    void set_default_lect_id(int p_lect_id) { default_lect_id_ = p_lect_id; core_dirty_ = true; emit_changed(); }
    [[nodiscard]] int get_default_lect_id() const noexcept { return default_lect_id_; }

    void set_sememe_resources(const godot::TypedArray<SememeResource> &p_sememes);
    [[nodiscard]] godot::TypedArray<SememeResource> get_sememe_resources() const { return sememe_resources_; }

    void set_semantic_frame_resources(const godot::TypedArray<SemanticFrameResource> &p_frames);
    [[nodiscard]] godot::TypedArray<SemanticFrameResource> get_semantic_frame_resources() const { return semantic_frame_resources_; }

    void set_morpheme_entries(const godot::Array &p_entries);
    [[nodiscard]] godot::Array get_morpheme_entries() const { return morpheme_entries_; }

    void set_lexeme_entries(const godot::Array &p_entries);
    [[nodiscard]] godot::Array get_lexeme_entries() const { return lexeme_entries_; }

    [[nodiscard]] const core::SemanticLexiconCore &get_core_lexicon() const;
    [[nodiscard]] core::SemanticLexiconCore &get_core_lexicon_mut();

    // --- Authoring & Registration API ---
    void clear_lexicon();
    void populate_default_english_ipa_inventory();
    int register_sememe_resource(const godot::Ref<SememeResource> &p_sememe);
    int register_semantic_frame_resource(const godot::Ref<SemanticFrameResource> &p_frame);
    int register_morpheme_entry(
        const godot::String &p_form,
        int p_kind,
        const godot::String &p_primary_seme_tag,
        bool p_negates_root = false,
        int p_category_shift = 0
    );
    int register_lexeme_entry(
        const godot::String &p_surface_form,
        const godot::StringName &p_sememe_label,
        int p_lexical_category = static_cast<int>(core::UniversalPOS::NOUN),
        int p_lect_id = 0,
        int p_register_formality = 128,
        float p_corpus_frequency = 1.0f,
        int p_slm_bpe_token_id = -1
    );
    bool register_synset_relation(
        const godot::StringName &p_source_label,
        const godot::StringName &p_target_label,
        int p_relation_type = static_cast<int>(core::WordNetRelationType::WN_HYPERNYM),
        float p_weight = 1.0f
    );

    // --- External NLP Standard Importers & Exporters ---
    int import_cmudict_text(const godot::String &p_cmudict_text, int p_lect_id = 0);
    int import_huggingface_tokenizer_json(const godot::String &p_json_text);
    int import_sentencepiece_vocab_text(const godot::String &p_tsv_text);
    int import_wordnet_synsets_json(const godot::String &p_json_text);
    int import_mrl_embeddings_json(const godot::String &p_json_text);

    [[nodiscard]] godot::PackedInt32Array tokenize_bpe_subwords(const godot::String &p_text) const;
    [[nodiscard]] godot::PackedStringArray tokenize_bpe_pieces(const godot::String &p_text) const;
    [[nodiscard]] godot::String decode_bpe_tokens(const godot::PackedInt32Array &p_token_ids) const;
    [[nodiscard]] godot::String export_gbnf_grammar(
        const godot::Ref<SemanticTagSetResource> &p_context_tags,
        int p_lect_id = 0,
        float p_min_affinity = 0.15f
    ) const;

    // --- Queries, Phonology, Morphology, WordNet & FrameNet ---
    [[nodiscard]] int get_phoneme_count() const;
    [[nodiscard]] int get_seme_count() const;
    [[nodiscard]] int get_sememe_count() const;
    [[nodiscard]] int get_morpheme_count() const;
    [[nodiscard]] int get_lexeme_count() const;
    [[nodiscard]] int get_bpe_vocab_count() const;
    [[nodiscard]] int get_synset_relation_count() const;
    [[nodiscard]] int get_semantic_frame_count() const;

    [[nodiscard]] godot::Ref<SememeResource> find_sememe_resource_by_label(const godot::StringName &p_label) const;
    [[nodiscard]] godot::Ref<SemanticFrameResource> find_semantic_frame_resource_by_name(const godot::StringName &p_name) const;
    [[nodiscard]] bool is_sememe_hypernym_of(
        const godot::StringName &p_hypernym_label,
        const godot::StringName &p_hyponym_label,
        int p_max_depth = 6
    ) const;

    [[nodiscard]] godot::PackedInt32Array encode_word_to_phoneme_masks(const godot::String &p_word) const;
    [[nodiscard]] float compute_phonetic_distance_between_words(
        const godot::String &p_word_a,
        const godot::String &p_word_b
    ) const;
    [[nodiscard]] godot::String decode_noisy_phoneme_masks_to_surface(
        const godot::PackedInt32Array &p_noisy_masks,
        int p_expected_sememe_id = -1,
        int p_preferred_lect_id = 0
    ) const;
    [[nodiscard]] godot::Dictionary decompose_word_into_morphemes(const godot::String &p_word) const;

    // --- Undo / Redo Snapshot ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
};

class LanguageAssetResource : public godot::Resource {
    GDCLASS(LanguageAssetResource, godot::Resource)

private:
    godot::String language_name_ = "Common";
    godot::String language_code_ = "en";
    int default_lect_id_         = 0;
    godot::TypedArray<LexiconResource> lexicons_;
    godot::TypedArray<GrammarRuleResource> grammar_rules_;
    godot::Ref<IdeamUndoRedo> undo_redo_;

    mutable core::SemanticLexiconCore unified_core_;
    mutable bool unified_dirty_ = true;

    void _rebuild_unified_core_if_dirty() const;
    godot::Ref<LexiconResource> _ensure_primary_lexicon();

protected:
    static void _bind_methods();

public:
    LanguageAssetResource() = default;
    ~LanguageAssetResource() override = default;

    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) { undo_redo_ = p_undo_redo; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_language_name(const godot::String &p_name) { language_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_language_name() const { return language_name_; }

    void set_language_code(const godot::String &p_code) { language_code_ = p_code; emit_changed(); }
    [[nodiscard]] godot::String get_language_code() const { return language_code_; }

    void set_default_lect_id(int p_lect) { default_lect_id_ = p_lect; unified_dirty_ = true; emit_changed(); }
    [[nodiscard]] int get_default_lect_id() const noexcept { return default_lect_id_; }

    void set_lexicons(const godot::TypedArray<LexiconResource> &p_lexicons);
    [[nodiscard]] godot::TypedArray<LexiconResource> get_lexicons() const { return lexicons_; }
    [[nodiscard]] godot::Ref<LexiconResource> get_lexicon() const {
        return lexicons_.size() > 0 ? godot::Ref<LexiconResource>(lexicons_[0]) : godot::Ref<LexiconResource>();
    }

    void set_grammar_rules(const godot::TypedArray<GrammarRuleResource> &p_rules);
    [[nodiscard]] godot::TypedArray<GrammarRuleResource> get_grammar_rules() const { return grammar_rules_; }

    void add_lexicon(const godot::Ref<LexiconResource> &p_lexicon);
    void add_grammar_rule(const godot::Ref<GrammarRuleResource> &p_rule);

    // --- Built-In Engine, WordNet, FrameNet, BPE & Wizard Vocabulary Bootstrapper ---
    void populate_builtin_godot_and_wizard_vocabulary();
    [[nodiscard]] const core::SemanticLexiconCore &get_unified_core_lexicon() const;

    // --- External NLP Importers & Exporters ---
    int import_cmudict_text(const godot::String &p_cmudict_text, int p_lect_id = 0);
    int import_huggingface_tokenizer_json(const godot::String &p_json_text);
    int import_sentencepiece_vocab_text(const godot::String &p_tsv_text);
    int import_wordnet_synsets_json(const godot::String &p_json_text);
    int import_mrl_embeddings_json(const godot::String &p_json_text);

    [[nodiscard]] godot::PackedInt32Array tokenize_bpe_subwords(const godot::String &p_text) const;
    [[nodiscard]] godot::PackedStringArray tokenize_bpe_pieces(const godot::String &p_text) const;
    [[nodiscard]] godot::String decode_bpe_tokens(const godot::PackedInt32Array &p_token_ids) const;
    [[nodiscard]] godot::String export_gbnf_grammar(
        const godot::Ref<SemanticTagSetResource> &p_context_tags,
        int p_lect_id = 0,
        float p_min_affinity = 0.15f
    ) const;

    // --- Morphological, Phonetic, Grammatical, WordNet, FrameNet & SLM Services ---
    [[nodiscard]] godot::Dictionary decompose_word_morphology(const godot::String &p_word) const;
    [[nodiscard]] godot::Ref<SememeResource> lookup_or_infer_sememe_for_token(const godot::String &p_token) const;
    [[nodiscard]] godot::Ref<SemanticFrameResource> find_semantic_frame_for_verb(const godot::String &p_verb) const;
    [[nodiscard]] bool is_sememe_hypernym_of(
        const godot::StringName &p_hypernym_label,
        const godot::StringName &p_hyponym_label,
        int p_max_depth = 6
    ) const;
    [[nodiscard]] godot::PackedInt32Array encode_text_to_phoneme_masks(
        const godot::String &p_text,
        int p_lect_id = 0
    ) const;
    [[nodiscard]] godot::String decode_noisy_phonemes_to_surface(
        const godot::PackedInt32Array &p_noisy_masks,
        int p_expected_sememe_id = -1,
        int p_lect_id = 0
    ) const;
    [[nodiscard]] godot::String apply_cascading_grammar(
        const godot::String &p_base_word,
        int p_lexical_category,
        int p_lect_id,
        const godot::PackedStringArray &p_active_seme_tags
    ) const;
    [[nodiscard]] godot::Dictionary compile_slm_vocabulary_constraints(
        const godot::Ref<SemanticTagSetResource> &p_context_tags,
        int p_lect_id = 0,
        int p_previous_lexical_category = 0,
        float p_min_affinity = 0.20f
    ) const;

    // --- Undo / Redo Snapshot ---
    [[nodiscard]] godot::Dictionary get_snapshot_state() const;
    void apply_snapshot_state(const godot::Dictionary &p_state);
    void commit_snapshot_mutation(const godot::String &p_action_name, const godot::Dictionary &p_before_state);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GrammarCascadeLayer);
VARIANT_ENUM_CAST(ideam::godot_ext::GrammarRuleOperation);
