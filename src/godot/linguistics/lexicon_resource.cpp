#include "lexicon_resource.h"
#include <godot_cpp/classes/json.hpp>
#include <algorithm>
#include <vector>

namespace ideam::godot_ext {

// ============================================================================
// GrammarRuleResource Implementation
// ============================================================================

void GrammarRuleResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_BASE_SYNTAX);
    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_MORPHOLOGY);
    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_DIALECT_LECT);
    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_REGISTER_FORMALITY);
    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_SLM_CONSTRAINT);
    BIND_ENUM_CONSTANT(GRAMMAR_LAYER_SPEAKER_IDIOLECT);

    BIND_ENUM_CONSTANT(GRAMMAR_OP_AFFIX_PREFIX);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_AFFIX_SUFFIX);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_SUBSTITUTE_STEM);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_CONSTRAIN_SLM_NEXT_CATEGORY);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_SHIFT_REGISTER_FORMALITY);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_INJECT_PERIPHERAL_SEME);
    BIND_ENUM_CONSTANT(GRAMMAR_OP_EMIT_GBNF_PRODUCTION);

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &GrammarRuleResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &GrammarRuleResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_rule_name", "rule_name"), &GrammarRuleResource::set_rule_name);
    ClassDB::bind_method(D_METHOD("get_rule_name"), &GrammarRuleResource::get_rule_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "rule_name"), "set_rule_name", "get_rule_name");

    ClassDB::bind_method(D_METHOD("set_priority", "priority"), &GrammarRuleResource::set_priority);
    ClassDB::bind_method(D_METHOD("get_priority"), &GrammarRuleResource::get_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

    ClassDB::bind_method(D_METHOD("set_is_important", "is_important"), &GrammarRuleResource::set_is_important);
    ClassDB::bind_method(D_METHOD("get_is_important"), &GrammarRuleResource::get_is_important);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_important"), "set_is_important", "get_is_important");

    ClassDB::bind_method(D_METHOD("set_cascade_layer", "layer"), &GrammarRuleResource::set_cascade_layer);
    ClassDB::bind_method(D_METHOD("get_cascade_layer"), &GrammarRuleResource::get_cascade_layer);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "cascade_layer",
            PROPERTY_HINT_ENUM,
            "Base Syntax:0,Morphology:1,Dialect Lect:2,Register Formality:3,SLM Constraint:4,Speaker Idiolect:5"
        ),
        "set_cascade_layer",
        "get_cascade_layer"
    );

    ClassDB::bind_method(D_METHOD("set_operation", "operation"), &GrammarRuleResource::set_operation);
    ClassDB::bind_method(D_METHOD("get_operation"), &GrammarRuleResource::get_operation);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "operation",
            PROPERTY_HINT_ENUM,
            "Affix Prefix:0,Affix Suffix:1,Substitute Stem:2,Constrain SLM Next Category:3,Shift Register Formality:4,Inject Peripheral Seme:5,Emit GBNF Production:6"
        ),
        "set_operation",
        "get_operation"
    );

    ClassDB::bind_method(D_METHOD("set_target_lexical_category", "category"), &GrammarRuleResource::set_target_lexical_category);
    ClassDB::bind_method(D_METHOD("get_target_lexical_category"), &GrammarRuleResource::get_target_lexical_category);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "target_lexical_category",
            PROPERTY_HINT_ENUM,
            "Any/X:0,NOUN:1,VERB:2,ADJ:3,ADV:4,ADP:5,PRON:6,DET:7,CCONJ:8,PART:9,PROPN:10,NUM:11,AUX:12,SCONJ:13,INTJ:14,SYM:15,PUNCT:16"
        ),
        "set_target_lexical_category",
        "get_target_lexical_category"
    );

    ClassDB::bind_method(D_METHOD("set_target_lect_id", "lect_id"), &GrammarRuleResource::set_target_lect_id);
    ClassDB::bind_method(D_METHOD("get_target_lect_id"), &GrammarRuleResource::get_target_lect_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_lect_id"), "set_target_lect_id", "get_target_lect_id");

    ClassDB::bind_method(D_METHOD("set_required_seme_tag", "tag"), &GrammarRuleResource::set_required_seme_tag);
    ClassDB::bind_method(D_METHOD("get_required_seme_tag"), &GrammarRuleResource::get_required_seme_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "required_seme_tag"), "set_required_seme_tag", "get_required_seme_tag");

    ClassDB::bind_method(D_METHOD("set_forbidden_seme_tag", "tag"), &GrammarRuleResource::set_forbidden_seme_tag);
    ClassDB::bind_method(D_METHOD("get_forbidden_seme_tag"), &GrammarRuleResource::get_forbidden_seme_tag);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "forbidden_seme_tag"), "set_forbidden_seme_tag", "get_forbidden_seme_tag");

    ClassDB::bind_method(D_METHOD("set_match_stem_pattern", "pattern"), &GrammarRuleResource::set_match_stem_pattern);
    ClassDB::bind_method(D_METHOD("get_match_stem_pattern"), &GrammarRuleResource::get_match_stem_pattern);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "match_stem_pattern"), "set_match_stem_pattern", "get_match_stem_pattern");

    ClassDB::bind_method(D_METHOD("set_affix_or_replacement", "affix"), &GrammarRuleResource::set_affix_or_replacement);
    ClassDB::bind_method(D_METHOD("get_affix_or_replacement"), &GrammarRuleResource::get_affix_or_replacement);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "affix_or_replacement"), "set_affix_or_replacement", "get_affix_or_replacement");

    ClassDB::bind_method(D_METHOD("set_gbnf_production_pattern", "gbnf_pattern"), &GrammarRuleResource::set_gbnf_production_pattern);
    ClassDB::bind_method(D_METHOD("get_gbnf_production_pattern"), &GrammarRuleResource::get_gbnf_production_pattern);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "gbnf_production_pattern"), "set_gbnf_production_pattern", "get_gbnf_production_pattern");

    ClassDB::bind_method(D_METHOD("set_slm_allowed_category_mask", "mask"), &GrammarRuleResource::set_slm_allowed_category_mask);
    ClassDB::bind_method(D_METHOD("get_slm_allowed_category_mask"), &GrammarRuleResource::get_slm_allowed_category_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "slm_allowed_category_mask"), "set_slm_allowed_category_mask", "get_slm_allowed_category_mask");

    ClassDB::bind_method(D_METHOD("set_formality_delta", "delta"), &GrammarRuleResource::set_formality_delta);
    ClassDB::bind_method(D_METHOD("get_formality_delta"), &GrammarRuleResource::get_formality_delta);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "formality_delta"), "set_formality_delta", "get_formality_delta");

    ClassDB::bind_method(D_METHOD("set_child_rules", "child_rules"), &GrammarRuleResource::set_child_rules);
    ClassDB::bind_method(D_METHOD("get_child_rules"), &GrammarRuleResource::get_child_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "child_rules", PROPERTY_HINT_ARRAY_TYPE, "GrammarRuleResource"),
        "set_child_rules",
        "get_child_rules"
    );

    ClassDB::bind_method(D_METHOD("get_specificity"), &GrammarRuleResource::get_specificity);
    ClassDB::bind_method(D_METHOD("outweighs", "other"), &GrammarRuleResource::outweighs);
    ClassDB::bind_method(
        D_METHOD("matches_context", "lexical_category", "lect_id", "active_seme_tags", "surface_word"),
        &GrammarRuleResource::matches_context,
        DEFVAL("")
    );
    ClassDB::bind_method(D_METHOD("apply_to_surface_word", "word"), &GrammarRuleResource::apply_to_surface_word);
    ClassDB::bind_method(D_METHOD("collect_cascaded_rules"), &GrammarRuleResource::collect_cascaded_rules);
    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &GrammarRuleResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &GrammarRuleResource::apply_snapshot_state);
}

void GrammarRuleResource::set_cascade_layer(int p_layer) {
    cascade_layer_ = std::clamp(p_layer, 0, 5);
    emit_changed();
}

void GrammarRuleResource::set_operation(int p_op) {
    operation_ = std::clamp(p_op, 0, 6);
    emit_changed();
}

void GrammarRuleResource::set_target_lexical_category(int p_cat) {
    target_lexical_category_ = std::clamp(p_cat, 0, 16);
    emit_changed();
}

void GrammarRuleResource::set_target_lect_id(int p_lect) {
    target_lect_id_ = std::max(0, p_lect);
    emit_changed();
}

int GrammarRuleResource::get_specificity() const {
    int score = 0;
    if (target_lexical_category_ != 0)   score += 1;
    if (target_lect_id_ != 0)            score += 2;
    if (!required_seme_tag_.is_empty())  score += 2;
    if (!forbidden_seme_tag_.is_empty()) score += 1;
    if (!match_stem_pattern_.is_empty()) score += 3;
    return score;
}

bool GrammarRuleResource::outweighs(const godot::Ref<GrammarRuleResource> &p_other) const {
    if (p_other.is_null()) return true;
    if (is_important_ != p_other->get_is_important()) {
        return is_important_;
    }
    if (cascade_layer_ != p_other->get_cascade_layer()) {
        return cascade_layer_ > p_other->get_cascade_layer();
    }
    if (priority_ != p_other->get_priority()) {
        return priority_ > p_other->get_priority();
    }
    return get_specificity() >= p_other->get_specificity();
}

bool GrammarRuleResource::matches_context(
    int p_lexical_category,
    int p_lect_id,
    const godot::PackedStringArray &p_active_seme_tags,
    const godot::String &p_surface_word
) const {
    if (target_lexical_category_ != 0 && target_lexical_category_ != p_lexical_category) {
        return false;
    }
    if (target_lect_id_ != 0 && target_lect_id_ != p_lect_id) {
        return false;
    }
    if (!required_seme_tag_.is_empty() && !p_active_seme_tags.has(godot::String(required_seme_tag_))) {
        return false;
    }
    if (!forbidden_seme_tag_.is_empty() && p_active_seme_tags.has(godot::String(forbidden_seme_tag_))) {
        return false;
    }
    if (!match_stem_pattern_.is_empty() && !p_surface_word.is_empty()) {
        if (!p_surface_word.matchn(match_stem_pattern_) &&
            !p_surface_word.to_lower().contains(match_stem_pattern_.to_lower())) {
            return false;
        }
    }
    return true;
}

godot::String GrammarRuleResource::apply_to_surface_word(const godot::String &p_word) const {
    if (p_word.is_empty() || affix_or_replacement_.is_empty()) {
        return p_word;
    }
    switch (static_cast<GrammarRuleOperation>(operation_)) {
        case GRAMMAR_OP_AFFIX_PREFIX:
            return affix_or_replacement_ + p_word;
        case GRAMMAR_OP_AFFIX_SUFFIX:
            return p_word + affix_or_replacement_;
        case GRAMMAR_OP_SUBSTITUTE_STEM:
            if (!match_stem_pattern_.is_empty()) {
                return p_word.replacen(match_stem_pattern_, affix_or_replacement_);
            }
            return affix_or_replacement_;
        default:
            return p_word;
    }
}

godot::TypedArray<GrammarRuleResource> GrammarRuleResource::collect_cascaded_rules() const {
    godot::TypedArray<GrammarRuleResource> out;
    out.push_back(const_cast<GrammarRuleResource *>(this));
    for (int i = 0; i < child_rules_.size(); ++i) {
        godot::Ref<GrammarRuleResource> child = child_rules_[i];
        if (child.is_valid()) {
            godot::TypedArray<GrammarRuleResource> sub = child->collect_cascaded_rules();
            for (int j = 0; j < sub.size(); ++j) {
                out.push_back(sub[j]);
            }
        }
    }
    return out;
}

godot::Dictionary GrammarRuleResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["rule_name"]                 = rule_name_;
    d["priority"]                  = priority_;
    d["is_important"]              = is_important_;
    d["cascade_layer"]             = cascade_layer_;
    d["operation"]                 = operation_;
    d["target_lexical_category"]   = target_lexical_category_;
    d["target_lect_id"]            = target_lect_id_;
    d["required_seme_tag"]         = required_seme_tag_;
    d["forbidden_seme_tag"]        = forbidden_seme_tag_;
    d["match_stem_pattern"]        = match_stem_pattern_;
    d["affix_or_replacement"]      = affix_or_replacement_;
    d["gbnf_production_pattern"]   = gbnf_production_pattern_;
    d["slm_allowed_category_mask"] = slm_allowed_category_mask_;
    d["formality_delta"]           = formality_delta_;
    d["child_rules"]               = child_rules_.duplicate();
    return d;
}

void GrammarRuleResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("rule_name"))                 rule_name_                 = p_state["rule_name"];
    if (p_state.has("priority"))                  priority_                  = static_cast<int>(p_state["priority"]);
    if (p_state.has("is_important"))              is_important_              = static_cast<bool>(p_state["is_important"]);
    if (p_state.has("cascade_layer"))             cascade_layer_             = static_cast<int>(p_state["cascade_layer"]);
    if (p_state.has("operation"))                 operation_                 = static_cast<int>(p_state["operation"]);
    if (p_state.has("target_lexical_category"))   target_lexical_category_   = static_cast<int>(p_state["target_lexical_category"]);
    if (p_state.has("target_lect_id"))            target_lect_id_            = static_cast<int>(p_state["target_lect_id"]);
    if (p_state.has("required_seme_tag"))         required_seme_tag_         = p_state["required_seme_tag"];
    if (p_state.has("forbidden_seme_tag"))        forbidden_seme_tag_        = p_state["forbidden_seme_tag"];
    if (p_state.has("match_stem_pattern"))        match_stem_pattern_        = p_state["match_stem_pattern"];
    if (p_state.has("affix_or_replacement"))      affix_or_replacement_      = p_state["affix_or_replacement"];
    if (p_state.has("gbnf_production_pattern"))   gbnf_production_pattern_   = p_state["gbnf_production_pattern"];
    if (p_state.has("slm_allowed_category_mask")) slm_allowed_category_mask_ = static_cast<int>(p_state["slm_allowed_category_mask"]);
    if (p_state.has("formality_delta"))           formality_delta_           = static_cast<int>(p_state["formality_delta"]);
    if (p_state.has("child_rules"))               child_rules_               = p_state["child_rules"];
    emit_changed();
}

void GrammarRuleResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) return;
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(after_state));
    undo_redo_->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    undo_redo_->commit_action(false);
}

// ============================================================================
// LexiconResource Implementation
// ============================================================================

LexiconResource::LexiconResource() {
    core_lexicon_.populate_default_core_semes_and_morphemes();
}

void LexiconResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &LexiconResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &LexiconResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_lexicon_name", "lexicon_name"), &LexiconResource::set_lexicon_name);
    ClassDB::bind_method(D_METHOD("get_lexicon_name"), &LexiconResource::get_lexicon_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "lexicon_name"), "set_lexicon_name", "get_lexicon_name");

    ClassDB::bind_method(D_METHOD("set_default_lect_id", "lect_id"), &LexiconResource::set_default_lect_id);
    ClassDB::bind_method(D_METHOD("get_default_lect_id"), &LexiconResource::get_default_lect_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_lect_id"), "set_default_lect_id", "get_default_lect_id");

    ClassDB::bind_method(D_METHOD("set_sememe_resources", "sememes"), &LexiconResource::set_sememe_resources);
    ClassDB::bind_method(D_METHOD("get_sememe_resources"), &LexiconResource::get_sememe_resources);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "sememe_resources", PROPERTY_HINT_ARRAY_TYPE, "SememeResource"),
        "set_sememe_resources",
        "get_sememe_resources"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_frame_resources", "frames"), &LexiconResource::set_semantic_frame_resources);
    ClassDB::bind_method(D_METHOD("get_semantic_frame_resources"), &LexiconResource::get_semantic_frame_resources);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "semantic_frame_resources", PROPERTY_HINT_ARRAY_TYPE, "SemanticFrameResource"),
        "set_semantic_frame_resources",
        "get_semantic_frame_resources"
    );

    ClassDB::bind_method(D_METHOD("set_morpheme_entries", "entries"), &LexiconResource::set_morpheme_entries);
    ClassDB::bind_method(D_METHOD("get_morpheme_entries"), &LexiconResource::get_morpheme_entries);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "morpheme_entries"), "set_morpheme_entries", "get_morpheme_entries");

    ClassDB::bind_method(D_METHOD("set_lexeme_entries", "entries"), &LexiconResource::set_lexeme_entries);
    ClassDB::bind_method(D_METHOD("get_lexeme_entries"), &LexiconResource::get_lexeme_entries);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "lexeme_entries"), "set_lexeme_entries", "get_lexeme_entries");

    ClassDB::bind_method(D_METHOD("clear_lexicon"), &LexiconResource::clear_lexicon);
    ClassDB::bind_method(D_METHOD("populate_default_english_ipa_inventory"), &LexiconResource::populate_default_english_ipa_inventory);
    ClassDB::bind_method(D_METHOD("register_sememe_resource", "sememe"), &LexiconResource::register_sememe_resource);
    ClassDB::bind_method(D_METHOD("register_semantic_frame_resource", "frame"), &LexiconResource::register_semantic_frame_resource);
    ClassDB::bind_method(
        D_METHOD("register_morpheme_entry", "form", "kind", "primary_seme_tag", "negates_root", "category_shift"),
        &LexiconResource::register_morpheme_entry,
        DEFVAL(false),
        DEFVAL(0)
    );
    ClassDB::bind_method(
        D_METHOD("register_lexeme_entry", "surface_form", "sememe_label", "lexical_category", "lect_id", "register_formality", "corpus_frequency", "slm_bpe_token_id"),
        &LexiconResource::register_lexeme_entry,
        DEFVAL(static_cast<int>(core::UniversalPOS::NOUN)),
        DEFVAL(0),
        DEFVAL(128),
        DEFVAL(1.0f),
        DEFVAL(-1)
    );
    ClassDB::bind_method(
        D_METHOD("register_synset_relation", "source_label", "target_label", "relation_type", "weight"),
        &LexiconResource::register_synset_relation,
        DEFVAL(static_cast<int>(core::WordNetRelationType::WN_HYPERNYM)),
        DEFVAL(1.0f)
    );

    // External NLP Importers & Exporters
    ClassDB::bind_method(
        D_METHOD("import_cmudict_text", "cmudict_text", "lect_id"),
        &LexiconResource::import_cmudict_text,
        DEFVAL(0)
    );
    ClassDB::bind_method(
        D_METHOD("import_huggingface_tokenizer_json", "json_text"),
        &LexiconResource::import_huggingface_tokenizer_json
    );
    ClassDB::bind_method(
        D_METHOD("import_sentencepiece_vocab_text", "tsv_text"),
        &LexiconResource::import_sentencepiece_vocab_text
    );
    ClassDB::bind_method(
        D_METHOD("import_wordnet_synsets_json", "json_text"),
        &LexiconResource::import_wordnet_synsets_json
    );
    ClassDB::bind_method(
        D_METHOD("import_mrl_embeddings_json", "json_text"),
        &LexiconResource::import_mrl_embeddings_json
    );

    ClassDB::bind_method(D_METHOD("tokenize_bpe_subwords", "text"), &LexiconResource::tokenize_bpe_subwords);
    ClassDB::bind_method(D_METHOD("tokenize_bpe_pieces", "text"), &LexiconResource::tokenize_bpe_pieces);
    ClassDB::bind_method(D_METHOD("decode_bpe_tokens", "token_ids"), &LexiconResource::decode_bpe_tokens);
    ClassDB::bind_method(
        D_METHOD("export_gbnf_grammar", "context_tags", "lect_id", "min_affinity"),
        &LexiconResource::export_gbnf_grammar,
        DEFVAL(0),
        DEFVAL(0.15f)
    );

    ClassDB::bind_method(D_METHOD("get_phoneme_count"), &LexiconResource::get_phoneme_count);
    ClassDB::bind_method(D_METHOD("get_seme_count"), &LexiconResource::get_seme_count);
    ClassDB::bind_method(D_METHOD("get_sememe_count"), &LexiconResource::get_sememe_count);
    ClassDB::bind_method(D_METHOD("get_morpheme_count"), &LexiconResource::get_morpheme_count);
    ClassDB::bind_method(D_METHOD("get_lexeme_count"), &LexiconResource::get_lexeme_count);
    ClassDB::bind_method(D_METHOD("get_bpe_vocab_count"), &LexiconResource::get_bpe_vocab_count);
    ClassDB::bind_method(D_METHOD("get_synset_relation_count"), &LexiconResource::get_synset_relation_count);
    ClassDB::bind_method(D_METHOD("get_semantic_frame_count"), &LexiconResource::get_semantic_frame_count);

    ClassDB::bind_method(D_METHOD("find_sememe_resource_by_label", "label"), &LexiconResource::find_sememe_resource_by_label);
    ClassDB::bind_method(D_METHOD("find_semantic_frame_resource_by_name", "name"), &LexiconResource::find_semantic_frame_resource_by_name);
    ClassDB::bind_method(
        D_METHOD("is_sememe_hypernym_of", "hypernym_label", "hyponym_label", "max_depth"),
        &LexiconResource::is_sememe_hypernym_of,
        DEFVAL(6)
    );
    ClassDB::bind_method(D_METHOD("encode_word_to_phoneme_masks", "word"), &LexiconResource::encode_word_to_phoneme_masks);
    ClassDB::bind_method(
        D_METHOD("compute_phonetic_distance_between_words", "word_a", "word_b"),
        &LexiconResource::compute_phonetic_distance_between_words
    );
    ClassDB::bind_method(
        D_METHOD("decode_noisy_phoneme_masks_to_surface", "noisy_masks", "expected_sememe_id", "preferred_lect_id"),
        &LexiconResource::decode_noisy_phoneme_masks_to_surface,
        DEFVAL(-1),
        DEFVAL(0)
    );
    ClassDB::bind_method(D_METHOD("decompose_word_into_morphemes", "word"), &LexiconResource::decompose_word_into_morphemes);

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &LexiconResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &LexiconResource::apply_snapshot_state);
}

void LexiconResource::_rebuild_core_if_dirty() const {
    if (!core_dirty_) {
        return;
    }
    core_lexicon_.clear();
    core_lexicon_.populate_default_core_semes_and_morphemes();

    for (int i = 0; i < sememe_resources_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememe_resources_[i];
        if (sem.is_valid()) {
            const godot::CharString lbl = godot::String(sem->get_canonical_label()).utf8();
            core_lexicon_.register_sememe(lbl.get_data(), sem->get_compiled_descriptor());
        }
    }

    // Wire WordNet hypernym/holonym edges declared on SememeResource properties
    for (int i = 0; i < sememe_resources_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememe_resources_[i];
        if (sem.is_null()) continue;
        const int32_t src_id = core_lexicon_.find_sememe_by_label(
            godot::String(sem->get_canonical_label()).utf8().get_data()
        );
        if (src_id < 0) continue;

        const godot::PackedStringArray hyps = sem->get_hypernym_labels();
        for (int j = 0; j < hyps.size(); ++j) {
            const int32_t tgt_id = core_lexicon_.find_sememe_by_label(hyps[j].utf8().get_data());
            if (tgt_id >= 0) {
                core_lexicon_.register_synset_relation(
                    static_cast<uint32_t>(src_id),
                    static_cast<uint32_t>(tgt_id),
                    core::WordNetRelationType::WN_HYPERNYM,
                    1.0f
                );
            }
        }
        const godot::PackedStringArray hols = sem->get_holonym_labels();
        for (int j = 0; j < hols.size(); ++j) {
            const int32_t tgt_id = core_lexicon_.find_sememe_by_label(hols[j].utf8().get_data());
            if (tgt_id >= 0) {
                core_lexicon_.register_synset_relation(
                    static_cast<uint32_t>(src_id),
                    static_cast<uint32_t>(tgt_id),
                    core::WordNetRelationType::WN_PART_HOLONYM,
                    1.0f
                );
            }
        }
    }

    // Explicit synset relation entries
    for (int i = 0; i < synset_relation_entries_.size(); ++i) {
        if (synset_relation_entries_[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d        = synset_relation_entries_[i];
        const godot::String src_lb = d.get("source_label", "");
        const godot::String tgt_lb = d.get("target_label", "");
        const int rel_type         = static_cast<int>(d.get("relation_type", 0));
        const float weight         = static_cast<float>(d.get("weight", 1.0f));
        const int32_t src_id       = core_lexicon_.find_sememe_by_label(src_lb.utf8().get_data());
        const int32_t tgt_id       = core_lexicon_.find_sememe_by_label(tgt_lb.utf8().get_data());
        if (src_id >= 0 && tgt_id >= 0) {
            core_lexicon_.register_synset_relation(
                static_cast<uint32_t>(src_id),
                static_cast<uint32_t>(tgt_id),
                static_cast<core::WordNetRelationType>(std::clamp(rel_type, 0, 9)),
                weight
            );
        }
    }

    // FrameNet SemanticFrameResources
    for (int i = 0; i < semantic_frame_resources_.size(); ++i) {
        godot::Ref<SemanticFrameResource> fr = semantic_frame_resources_[i];
        if (fr.is_valid()) {
            core_lexicon_.register_semantic_frame(godot::String(fr->get_frame_name()).utf8().get_data(), fr->get_compiled_frame_pod());
        }
    }

    for (int i = 0; i < morpheme_entries_.size(); ++i) {
        if (morpheme_entries_[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d      = morpheme_entries_[i];
        const godot::String form = d.get("form", "");
        const int kind           = static_cast<int>(d.get("kind", 0));
        const godot::String tag  = d.get("primary_seme_tag", "");
        const bool negates       = static_cast<bool>(d.get("negates_root", false));
        const int cat_shift      = static_cast<int>(d.get("category_shift", 0));
        const uint32_t bit       = core_lexicon_.resolve_seme_bit(tag.utf8().get_data());
        const uint64_t mod_mask  = (bit < 64u) ? (1ULL << bit) : 0ULL;
        core_lexicon_.register_morpheme(
            form.utf8().get_data(),
            static_cast<core::MorphemeKind>(std::clamp(kind, 0, 4)),
            static_cast<uint16_t>(bit),
            mod_mask,
            negates,
            static_cast<core::UniversalPOS>(std::clamp(cat_shift, 0, 16))
        );
    }

    for (int i = 0; i < lexeme_entries_.size(); ++i) {
        if (lexeme_entries_[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d         = lexeme_entries_[i];
        const godot::String surface = d.get("surface_form", "");
        const godot::String sem_lbl = d.get("sememe_label", surface);
        const int category          = static_cast<int>(d.get("lexical_category", 1));
        const int lect_id           = static_cast<int>(d.get("lect_id", default_lect_id_));
        const int formality         = static_cast<int>(d.get("register_formality", 128));
        const float freq            = static_cast<float>(d.get("corpus_frequency", 1.0f));
        const int bpe_id            = static_cast<int>(d.get("slm_bpe_token_id", -1));
        const godot::String arpabet = d.get("arpabet_pronunciation", "");

        if (!arpabet.is_empty()) {
            core_lexicon_.import_cmudict_entry(
                surface.utf8().get_data(),
                arpabet.utf8().get_data(),
                static_cast<uint16_t>(std::max(0, lect_id)),
                static_cast<core::UniversalPOS>(std::clamp(category, 0, 16))
            );
            continue;
        }

        int32_t sem_idx = core_lexicon_.find_sememe_by_label(sem_lbl.utf8().get_data());
        if (sem_idx < 0) {
            core::SememeDescriptorPOD desc{};
            desc.set_nuclear_bit(core_lexicon_.resolve_seme_bit(sem_lbl.utf8().get_data()));
            desc.taxeme_category = static_cast<uint8_t>(std::clamp(category, 0, 16));
            core::SemanticLexiconCore::compute_character_ngram_simhash128(
                sem_lbl.utf8().get_data(),
                desc.nuclear_seme_words[1],
                desc.nuclear_seme_words[2]
            );
            sem_idx = static_cast<int32_t>(core_lexicon_.register_sememe(sem_lbl.utf8().get_data(), desc));
        }

        core_lexicon_.register_lexeme(
            surface.utf8().get_data(),
            static_cast<uint32_t>(sem_idx),
            static_cast<core::UniversalPOS>(std::clamp(category, 0, 16)),
            static_cast<uint16_t>(std::max(0, lect_id)),
            static_cast<uint8_t>(std::clamp(formality, 0, 255)),
            freq,
            (bpe_id >= 0) ? static_cast<uint32_t>(bpe_id) : 0xFFFFFFFFu
        );
    }

    // BPE Vocab & Merge Rules
    for (int i = 0; i < bpe_vocab_entries_.size(); ++i) {
        if (bpe_vocab_entries_[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d       = bpe_vocab_entries_[i];
        const godot::String piece = d.get("piece", "");
        const int token_id        = static_cast<int>(d.get("token_id", i));
        const float score         = static_cast<float>(d.get("score", 0.0f));
        const bool is_special     = static_cast<bool>(d.get("is_special", false));
        core_lexicon_.register_bpe_vocab_entry(
            piece.utf8().get_data(),
            static_cast<uint32_t>(std::max(0, token_id)),
            score,
            0xFFFFFFFFu,
            is_special
        );
    }

    for (int i = 0; i < bpe_merge_entries_.size(); ++i) {
        if (bpe_merge_entries_[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary d         = bpe_merge_entries_[i];
        const godot::String left_p  = d.get("left", "");
        const godot::String right_p = d.get("right", "");
        const int rank              = static_cast<int>(d.get("rank", i));
        const int32_t lid           = core_lexicon_.find_bpe_token_id(left_p.utf8().get_data());
        const int32_t rid           = core_lexicon_.find_bpe_token_id(right_p.utf8().get_data());
        const godot::String merged  = left_p + right_p;
        int32_t mid                 = core_lexicon_.find_bpe_token_id(merged.utf8().get_data());
        if (mid < 0) {
            mid = static_cast<int32_t>(core_lexicon_.register_bpe_vocab_entry(
                merged.utf8().get_data(),
                static_cast<uint32_t>(core_lexicon_.bpe_vocab().size()),
                0.0f
            ));
        }
        if (lid >= 0 && rid >= 0 && mid >= 0) {
            core_lexicon_.register_bpe_merge_rule(
                static_cast<uint32_t>(lid),
                static_cast<uint32_t>(rid),
                static_cast<uint32_t>(mid),
                static_cast<uint32_t>(std::max(0, rank))
            );
        }
    }

    core_dirty_ = false;
}

const core::SemanticLexiconCore &LexiconResource::get_core_lexicon() const {
    _rebuild_core_if_dirty();
    return core_lexicon_;
}

core::SemanticLexiconCore &LexiconResource::get_core_lexicon_mut() {
    _rebuild_core_if_dirty();
    return core_lexicon_;
}

void LexiconResource::set_sememe_resources(const godot::TypedArray<SememeResource> &p_sememes) {
    sememe_resources_ = p_sememes;
    core_dirty_       = true;
    emit_changed();
}

void LexiconResource::set_semantic_frame_resources(const godot::TypedArray<SemanticFrameResource> &p_frames) {
    semantic_frame_resources_ = p_frames;
    core_dirty_               = true;
    emit_changed();
}

void LexiconResource::set_morpheme_entries(const godot::Array &p_entries) {
    morpheme_entries_ = p_entries;
    core_dirty_       = true;
    emit_changed();
}

void LexiconResource::set_lexeme_entries(const godot::Array &p_entries) {
    lexeme_entries_ = p_entries;
    core_dirty_     = true;
    emit_changed();
}

void LexiconResource::clear_lexicon() {
    sememe_resources_.clear();
    semantic_frame_resources_.clear();
    morpheme_entries_.clear();
    lexeme_entries_.clear();
    synset_relation_entries_.clear();
    bpe_vocab_entries_.clear();
    bpe_merge_entries_.clear();
    core_lexicon_.clear();
    core_dirty_ = true;
    emit_changed();
}

void LexiconResource::populate_default_english_ipa_inventory() {
    _rebuild_core_if_dirty();
    core_lexicon_.populate_default_core_semes_and_morphemes();
    emit_changed();
}

int LexiconResource::register_sememe_resource(const godot::Ref<SememeResource> &p_sememe) {
    if (p_sememe.is_null()) return -1;
    if (!sememe_resources_.has(p_sememe)) {
        sememe_resources_.push_back(p_sememe);
    }
    core_dirty_ = true;
    _rebuild_core_if_dirty();
    const int32_t id = core_lexicon_.find_sememe_by_label(
        godot::String(p_sememe->get_canonical_label()).utf8().get_data()
    );
    emit_changed();
    return static_cast<int>(id);
}

int LexiconResource::register_semantic_frame_resource(const godot::Ref<SemanticFrameResource> &p_frame) {
    if (p_frame.is_null()) return -1;
    if (!semantic_frame_resources_.has(p_frame)) {
        semantic_frame_resources_.push_back(p_frame);
    }
    core_dirty_ = true;
    _rebuild_core_if_dirty();
    emit_changed();
    return static_cast<int>(core_lexicon_.semantic_frames().size()) - 1;
}

int LexiconResource::register_morpheme_entry(
    const godot::String &p_form,
    int p_kind,
    const godot::String &p_primary_seme_tag,
    bool p_negates_root,
    int p_category_shift
) {
    godot::Dictionary d;
    d["form"]             = p_form;
    d["kind"]             = p_kind;
    d["primary_seme_tag"] = p_primary_seme_tag;
    d["negates_root"]     = p_negates_root;
    d["category_shift"]   = p_category_shift;
    morpheme_entries_.push_back(d);
    core_dirty_ = true;
    _rebuild_core_if_dirty();
    emit_changed();
    return static_cast<int>(core_lexicon_.morphemes().size()) - 1;
}

int LexiconResource::register_lexeme_entry(
    const godot::String &p_surface_form,
    const godot::StringName &p_sememe_label,
    int p_lexical_category,
    int p_lect_id,
    int p_register_formality,
    float p_corpus_frequency,
    int p_slm_bpe_token_id
) {
    godot::Dictionary d;
    d["surface_form"]       = p_surface_form;
    d["sememe_label"]       = godot::String(p_sememe_label);
    d["lexical_category"]   = p_lexical_category;
    d["lect_id"]            = p_lect_id;
    d["register_formality"] = p_register_formality;
    d["corpus_frequency"]   = p_corpus_frequency;
    d["slm_bpe_token_id"]   = p_slm_bpe_token_id;
    lexeme_entries_.push_back(d);
    core_dirty_ = true;
    _rebuild_core_if_dirty();
    emit_changed();
    return static_cast<int>(core_lexicon_.lexemes().size()) - 1;
}

bool LexiconResource::register_synset_relation(
    const godot::StringName &p_source_label,
    const godot::StringName &p_target_label,
    int p_relation_type,
    float p_weight
) {
    if (p_source_label.is_empty() || p_target_label.is_empty()) {
        return false;
    }
    godot::Dictionary d;
    d["source_label"]  = godot::String(p_source_label);
    d["target_label"]  = godot::String(p_target_label);
    d["relation_type"] = p_relation_type;
    d["weight"]        = p_weight;
    synset_relation_entries_.push_back(d);
    core_dirty_ = true;
    _rebuild_core_if_dirty();
    emit_changed();
    return true;
}

int LexiconResource::import_cmudict_text(const godot::String &p_cmudict_text, int p_lect_id) {
    if (p_cmudict_text.is_empty()) return 0;
    const godot::PackedStringArray lines = p_cmudict_text.split("\n", false);
    int imported = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const godot::String line = lines[i].strip_edges();
        if (line.is_empty() || line.begins_with(";;;") || line.begins_with("#")) {
            continue;
        }
        int split_idx = line.find(" ");
        if (split_idx < 0) split_idx = line.find("\t");
        if (split_idx <= 0) continue;

        godot::String word = line.substr(0, split_idx).strip_edges().to_lower();
        const int paren_idx = word.find("(");
        if (paren_idx > 0) {
            word = word.substr(0, paren_idx);
        }
        const godot::String arpabet = line.substr(split_idx + 1).strip_edges();
        if (word.is_empty() || arpabet.is_empty()) continue;

        godot::Dictionary d;
        d["surface_form"]          = word;
        d["sememe_label"]          = word;
        d["lexical_category"]      = static_cast<int>(core::UniversalPOS::NOUN);
        d["lect_id"]               = p_lect_id;
        d["arpabet_pronunciation"] = arpabet;
        lexeme_entries_.push_back(d);
        ++imported;
    }
    if (imported > 0) {
        core_dirty_ = true;
        _rebuild_core_if_dirty();
        emit_changed();
    }
    return imported;
}

int LexiconResource::import_huggingface_tokenizer_json(const godot::String &p_json_text) {
    if (p_json_text.is_empty()) return 0;
    const godot::Variant parsed = godot::JSON::parse_string(p_json_text);
    if (parsed.get_type() != godot::Variant::DICTIONARY) {
        return 0;
    }
    godot::Dictionary root = parsed;
    godot::Dictionary model = root.has("model") && root["model"].get_type() == godot::Variant::DICTIONARY
        ? godot::Dictionary(root["model"])
        : root;

    int imported_count = 0;
    if (model.has("vocab") && model["vocab"].get_type() == godot::Variant::DICTIONARY) {
        godot::Dictionary vocab = model["vocab"];
        const godot::Array keys = vocab.keys();
        for (int i = 0; i < keys.size(); ++i) {
            const godot::String piece = keys[i];
            const int token_id = static_cast<int>(vocab[keys[i]]);
            godot::Dictionary entry;
            entry["piece"]      = piece;
            entry["token_id"]   = token_id;
            entry["score"]      = 0.0f;
            entry["is_special"] = piece.begins_with("<") && piece.ends_with(">");
            bpe_vocab_entries_.push_back(entry);
            ++imported_count;
        }
    }

    if (model.has("merges") && model["merges"].get_type() == godot::Variant::ARRAY) {
        godot::Array merges = model["merges"];
        for (int i = 0; i < merges.size(); ++i) {
            godot::String left_p;
            godot::String right_p;
            if (merges[i].get_type() == godot::Variant::STRING) {
                const godot::String pair_str = merges[i];
                const int sp = pair_str.find(" ");
                if (sp > 0) {
                    left_p  = pair_str.substr(0, sp);
                    right_p = pair_str.substr(sp + 1);
                }
            } else if (merges[i].get_type() == godot::Variant::ARRAY) {
                godot::Array pair_arr = merges[i];
                if (pair_arr.size() >= 2) {
                    left_p  = pair_arr[0];
                    right_p = pair_arr[1];
                }
            }
            if (!left_p.is_empty() && !right_p.is_empty()) {
                godot::Dictionary m;
                m["left"]  = left_p;
                m["right"] = right_p;
                m["rank"]  = i;
                bpe_merge_entries_.push_back(m);
            }
        }
    }

    if (imported_count > 0) {
        core_dirty_ = true;
        _rebuild_core_if_dirty();
        emit_changed();
    }
    return imported_count;
}

int LexiconResource::import_sentencepiece_vocab_text(const godot::String &p_tsv_text) {
    if (p_tsv_text.is_empty()) return 0;
    const godot::PackedStringArray lines = p_tsv_text.split("\n", false);
    int imported = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const godot::String line = lines[i].strip_edges();
        if (line.is_empty()) continue;
        const int tab_idx = line.find("\t");
        const godot::String piece = (tab_idx > 0) ? line.substr(0, tab_idx) : line;
        const float score = (tab_idx > 0) ? static_cast<float>(line.substr(tab_idx + 1).to_float()) : 0.0f;
        godot::Dictionary entry;
        entry["piece"]      = piece;
        entry["token_id"]   = static_cast<int>(bpe_vocab_entries_.size());
        entry["score"]      = score;
        entry["is_special"] = piece.begins_with("<") && piece.ends_with(">");
        bpe_vocab_entries_.push_back(entry);
        ++imported;
    }
    if (imported > 0) {
        core_dirty_ = true;
        _rebuild_core_if_dirty();
        emit_changed();
    }
    return imported;
}

int LexiconResource::import_wordnet_synsets_json(const godot::String &p_json_text) {
    if (p_json_text.is_empty()) return 0;
    const godot::Variant parsed = godot::JSON::parse_string(p_json_text);
    godot::Array items;
    if (parsed.get_type() == godot::Variant::ARRAY) {
        items = parsed;
    } else if (parsed.get_type() == godot::Variant::DICTIONARY) {
        godot::Dictionary d = parsed;
        if (d.has("synsets") && d["synsets"].get_type() == godot::Variant::ARRAY) {
            items = d["synsets"];
        }
    }

    int imported = 0;
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].get_type() != godot::Variant::DICTIONARY) continue;
        godot::Dictionary item = items[i];
        const godot::String label = item.get("label", item.get("lemma", ""));
        if (label.is_empty()) continue;

        godot::Ref<SememeResource> sem = find_sememe_resource_by_label(godot::StringName(label));
        if (sem.is_null()) {
            sem.instantiate();
            sem->set_canonical_label(godot::StringName(label));
        }
        if (item.has("synset_id")) {
            sem->set_wordnet_synset_id(static_cast<int>(item["synset_id"]));
        }
        if (item.has("gloss")) {
            sem->set_gloss_definition(item["gloss"]);
        }
        if (item.has("upos")) {
            if (item["upos"].get_type() == godot::Variant::STRING) {
                const godot::String upos_str = item["upos"];
                sem->set_upos_tag(static_cast<int>(core::parse_upos_tag(upos_str.utf8().get_data())));
            } else {
                sem->set_upos_tag(static_cast<int>(item["upos"]));
            }
        }
        if (item.has("nuclear_semes") && item["nuclear_semes"].get_type() == godot::Variant::ARRAY) {
            godot::Array arr = item["nuclear_semes"];
            for (int j = 0; j < arr.size(); ++j) {
                sem->add_nuclear_seme_tag(arr[j]);
            }
        }
        if (item.has("peripheral_semes") && item["peripheral_semes"].get_type() == godot::Variant::ARRAY) {
            godot::Array arr = item["peripheral_semes"];
            for (int j = 0; j < arr.size(); ++j) {
                sem->add_peripheral_seme_tag(arr[j]);
            }
        }
        if (item.has("hypernyms") && item["hypernyms"].get_type() == godot::Variant::ARRAY) {
            godot::Array arr = item["hypernyms"];
            godot::PackedStringArray hyps = sem->get_hypernym_labels();
            for (int j = 0; j < arr.size(); ++j) {
                const godot::String h = arr[j];
                if (!hyps.has(h)) hyps.push_back(h);
            }
            sem->set_hypernym_labels(hyps);
        }
        if (item.has("holonyms") && item["holonyms"].get_type() == godot::Variant::ARRAY) {
            godot::Array arr = item["holonyms"];
            godot::PackedStringArray hols = sem->get_holonym_labels();
            for (int j = 0; j < arr.size(); ++j) {
                const godot::String h = arr[j];
                if (!hols.has(h)) hols.push_back(h);
            }
            sem->set_holonym_labels(hols);
        }

        if (!sememe_resources_.has(sem)) {
            sememe_resources_.push_back(sem);
        }
        register_lexeme_entry(label, godot::StringName(label), sem->get_upos_tag(), default_lect_id_);
        ++imported;
    }

    if (imported > 0) {
        core_dirty_ = true;
        _rebuild_core_if_dirty();
        emit_changed();
    }
    return imported;
}

int LexiconResource::import_mrl_embeddings_json(const godot::String &p_json_text) {
    if (p_json_text.is_empty()) return 0;
    const godot::Variant parsed = godot::JSON::parse_string(p_json_text);
    if (parsed.get_type() != godot::Variant::DICTIONARY) return 0;
    godot::Dictionary dict = parsed;
    const godot::Array keys = dict.keys();
    int imported = 0;

    for (int i = 0; i < keys.size(); ++i) {
        const godot::String label = keys[i];
        if (dict[keys[i]].get_type() != godot::Variant::ARRAY) continue;
        godot::Array vec_arr = dict[keys[i]];
        godot::PackedFloat32Array floats;
        floats.resize(vec_arr.size());
        for (int j = 0; j < vec_arr.size(); ++j) {
            floats.set(j, static_cast<float>(vec_arr[j]));
        }

        godot::Ref<SememeResource> sem = find_sememe_resource_by_label(godot::StringName(label));
        if (sem.is_null()) {
            sem.instantiate();
            sem->set_canonical_label(godot::StringName(label));
            sem->add_nuclear_seme_tag(label);
            sememe_resources_.push_back(sem);
        }
        sem->set_mrl_float_embedding(floats);
        ++imported;
    }

    if (imported > 0) {
        core_dirty_ = true;
        _rebuild_core_if_dirty();
        emit_changed();
    }
    return imported;
}

godot::PackedInt32Array LexiconResource::tokenize_bpe_subwords(const godot::String &p_text) const {
    _rebuild_core_if_dirty();
    std::vector<uint32_t> ids;
    core_lexicon_.tokenize_bpe_subwords(p_text.utf8().get_data(), ids);
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(ids[i]));
    }
    return out;
}

godot::PackedStringArray LexiconResource::tokenize_bpe_pieces(const godot::String &p_text) const {
    _rebuild_core_if_dirty();
    std::vector<uint32_t> ids;
    core_lexicon_.tokenize_bpe_subwords(p_text.utf8().get_data(), ids);
    godot::PackedStringArray out;
    for (uint32_t id : ids) {
        std::string_view piece = core_lexicon_.get_bpe_token_piece(id);
        out.push_back(godot::String::utf8(piece.data(), static_cast<int>(piece.size())));
    }
    return out;
}

godot::String LexiconResource::decode_bpe_tokens(const godot::PackedInt32Array &p_token_ids) const {
    _rebuild_core_if_dirty();
    std::vector<uint32_t> ids(static_cast<size_t>(p_token_ids.size()));
    for (int64_t i = 0; i < p_token_ids.size(); ++i) {
        ids[static_cast<size_t>(i)] = static_cast<uint32_t>(std::max<int32_t>(0, p_token_ids[i]));
    }
    const std::string decoded = core_lexicon_.decode_bpe_tokens(ids);
    return godot::String::utf8(decoded.c_str(), static_cast<int>(decoded.size()));
}

godot::String LexiconResource::export_gbnf_grammar(
    const godot::Ref<SemanticTagSetResource> &p_context_tags,
    int p_lect_id,
    float p_min_affinity
) const {
    _rebuild_core_if_dirty();
    const core::SememeDescriptorPOD ctx_desc =
        p_context_tags.is_valid() ? p_context_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};
    const std::string gbnf = core_lexicon_.export_gbnf_grammar_for_constraints(
        ctx_desc,
        static_cast<uint16_t>(std::max(0, p_lect_id)),
        0xFFFFu,
        p_min_affinity
    );
    return godot::String::utf8(gbnf.c_str(), static_cast<int>(gbnf.size()));
}

int LexiconResource::get_phoneme_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.phonemes().size());
}

int LexiconResource::get_seme_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.semes().size());
}

int LexiconResource::get_sememe_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.sememes().size());
}

int LexiconResource::get_morpheme_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.morphemes().size());
}

int LexiconResource::get_lexeme_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.lexemes().size());
}

int LexiconResource::get_bpe_vocab_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.bpe_vocab().size());
}

int LexiconResource::get_synset_relation_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.synset_relations().size());
}

int LexiconResource::get_semantic_frame_count() const {
    _rebuild_core_if_dirty();
    return static_cast<int>(core_lexicon_.semantic_frames().size());
}

godot::Ref<SememeResource> LexiconResource::find_sememe_resource_by_label(
    const godot::StringName &p_label
) const {
    for (int i = 0; i < sememe_resources_.size(); ++i) {
        godot::Ref<SememeResource> sem = sememe_resources_[i];
        if (sem.is_valid() && sem->get_canonical_label() == p_label) {
            return sem;
        }
    }
    return {};
}

godot::Ref<SemanticFrameResource> LexiconResource::find_semantic_frame_resource_by_name(
    const godot::StringName &p_name
) const {
    for (int i = 0; i < semantic_frame_resources_.size(); ++i) {
        godot::Ref<SemanticFrameResource> fr = semantic_frame_resources_[i];
        if (fr.is_valid() && fr->get_frame_name() == p_name) {
            return fr;
        }
    }
    return {};
}

bool LexiconResource::is_sememe_hypernym_of(
    const godot::StringName &p_hypernym_label,
    const godot::StringName &p_hyponym_label,
    int p_max_depth
) const {
    _rebuild_core_if_dirty();
    const int32_t hyp_id = core_lexicon_.find_sememe_by_label(godot::String(p_hypernym_label).utf8().get_data());
    const int32_t sub_id = core_lexicon_.find_sememe_by_label(godot::String(p_hyponym_label).utf8().get_data());
    if (hyp_id < 0 || sub_id < 0) return false;
    return core_lexicon_.is_hypernym_of(
        static_cast<uint32_t>(hyp_id),
        static_cast<uint32_t>(sub_id),
        static_cast<uint32_t>(std::max(1, p_max_depth))
    );
}

godot::PackedInt32Array LexiconResource::encode_word_to_phoneme_masks(const godot::String &p_word) const {
    _rebuild_core_if_dirty();
    std::vector<uint16_t> masks;
    const godot::CharString cs = p_word.utf8();
    const int32_t lx_id = core_lexicon_.find_lexeme_by_surface(cs.get_data(), static_cast<uint16_t>(default_lect_id_));
    if (lx_id >= 0) {
        std::span<const uint16_t> slice = core_lexicon_.get_lexeme_phoneme_masks(static_cast<uint32_t>(lx_id));
        masks.assign(slice.begin(), slice.end());
    } else {
        core_lexicon_.approximate_phonemes_from_text(cs.get_data(), masks);
    }

    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(masks.size()));
    for (size_t i = 0; i < masks.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(masks[i]));
    }
    return out;
}

float LexiconResource::compute_phonetic_distance_between_words(
    const godot::String &p_word_a,
    const godot::String &p_word_b
) const {
    _rebuild_core_if_dirty();
    std::vector<uint16_t> masks_a;
    std::vector<uint16_t> masks_b;
    core_lexicon_.approximate_phonemes_from_text(p_word_a.utf8().get_data(), masks_a);
    core_lexicon_.approximate_phonemes_from_text(p_word_b.utf8().get_data(), masks_b);
    return core::SemanticLexiconCore::compute_phoneme_slice_distance(masks_a, masks_b);
}

godot::String LexiconResource::decode_noisy_phoneme_masks_to_surface(
    const godot::PackedInt32Array &p_noisy_masks,
    int p_expected_sememe_id,
    int p_preferred_lect_id
) const {
    _rebuild_core_if_dirty();
    std::vector<uint16_t> masks(static_cast<size_t>(p_noisy_masks.size()));
    for (int64_t i = 0; i < p_noisy_masks.size(); ++i) {
        masks[static_cast<size_t>(i)] = static_cast<uint16_t>(p_noisy_masks[i] & 0xFFFF);
    }
    const uint32_t best_lx = core_lexicon_.find_best_matching_lexeme_from_noisy_phonemes(
        masks,
        (p_expected_sememe_id >= 0) ? static_cast<uint32_t>(p_expected_sememe_id) : 0xFFFFFFFFu,
        static_cast<uint16_t>(std::max(0, p_preferred_lect_id))
    );
    if (best_lx == 0xFFFFFFFFu) {
        return godot::String("...");
    }
    std::string_view surf = core_lexicon_.get_lexeme_surface(best_lx);
    return godot::String::utf8(surf.data(), static_cast<int>(surf.size()));
}

godot::Dictionary LexiconResource::decompose_word_into_morphemes(const godot::String &p_word) const {
    _rebuild_core_if_dirty();
    const auto res = core_lexicon_.decompose_word_morphemes(p_word.utf8().get_data());
    godot::Dictionary d;
    d["matched_lexeme_id"]  = (res.matched_lexeme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.matched_lexeme_id);
    d["matched_sememe_id"]  = (res.matched_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.matched_sememe_id);
    d["prefix_morpheme_id"] = (res.prefix_morpheme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.prefix_morpheme_id);
    d["suffix_morpheme_id"] = (res.suffix_morpheme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.suffix_morpheme_id);
    d["inferred_category"]  = static_cast<int>(res.inferred_category);
    d["upos_name"]          = godot::String(core::upos_to_string(res.inferred_category).data());
    d["is_negated"]         = res.is_negated;
    d["nuclear_popcount"]   = static_cast<int>(res.composed_descriptor.nuclear_popcount());
    return d;
}

godot::Dictionary LexiconResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["lexicon_name"]             = lexicon_name_;
    d["default_lect_id"]          = default_lect_id_;
    d["sememe_resources"]         = sememe_resources_.duplicate();
    d["semantic_frame_resources"] = semantic_frame_resources_.duplicate();
    d["morpheme_entries"]         = morpheme_entries_.duplicate(true);
    d["lexeme_entries"]           = lexeme_entries_.duplicate(true);
    d["synset_relation_entries"]  = synset_relation_entries_.duplicate(true);
    d["bpe_vocab_entries"]        = bpe_vocab_entries_.duplicate(true);
    d["bpe_merge_entries"]        = bpe_merge_entries_.duplicate(true);
    return d;
}

void LexiconResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("lexicon_name"))             lexicon_name_             = p_state["lexicon_name"];
    if (p_state.has("default_lect_id"))          default_lect_id_          = static_cast<int>(p_state["default_lect_id"]);
    if (p_state.has("sememe_resources"))         sememe_resources_         = p_state["sememe_resources"];
    if (p_state.has("semantic_frame_resources")) semantic_frame_resources_ = p_state["semantic_frame_resources"];
    if (p_state.has("morpheme_entries"))         morpheme_entries_         = p_state["morpheme_entries"];
    if (p_state.has("lexeme_entries"))           lexeme_entries_           = p_state["lexeme_entries"];
    if (p_state.has("synset_relation_entries"))  synset_relation_entries_  = p_state["synset_relation_entries"];
    if (p_state.has("bpe_vocab_entries"))        bpe_vocab_entries_        = p_state["bpe_vocab_entries"];
    if (p_state.has("bpe_merge_entries"))        bpe_merge_entries_        = p_state["bpe_merge_entries"];
    core_dirty_ = true;
    emit_changed();
}

void LexiconResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) return;
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(after_state));
    undo_redo_->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    undo_redo_->commit_action(false);
}

// ============================================================================
// LanguageAssetResource Implementation
// ============================================================================

void LanguageAssetResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_undo_redo", "undo_redo"), &LanguageAssetResource::set_undo_redo);
    ClassDB::bind_method(D_METHOD("get_undo_redo"), &LanguageAssetResource::get_undo_redo);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "undo_redo", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE),
        "set_undo_redo",
        "get_undo_redo"
    );

    ClassDB::bind_method(D_METHOD("set_language_name", "language_name"), &LanguageAssetResource::set_language_name);
    ClassDB::bind_method(D_METHOD("get_language_name"), &LanguageAssetResource::get_language_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "language_name"), "set_language_name", "get_language_name");

    ClassDB::bind_method(D_METHOD("set_language_code", "language_code"), &LanguageAssetResource::set_language_code);
    ClassDB::bind_method(D_METHOD("get_language_code"), &LanguageAssetResource::get_language_code);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "language_code"), "set_language_code", "get_language_code");

    ClassDB::bind_method(D_METHOD("set_default_lect_id", "lect_id"), &LanguageAssetResource::set_default_lect_id);
    ClassDB::bind_method(D_METHOD("get_default_lect_id"), &LanguageAssetResource::get_default_lect_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_lect_id"), "set_default_lect_id", "get_default_lect_id");

    ClassDB::bind_method(D_METHOD("set_lexicons", "lexicons"), &LanguageAssetResource::set_lexicons);
    ClassDB::bind_method(D_METHOD("get_lexicons"), &LanguageAssetResource::get_lexicons);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "lexicons", PROPERTY_HINT_ARRAY_TYPE, "LexiconResource"),
        "set_lexicons",
        "get_lexicons"
    );

    ClassDB::bind_method(D_METHOD("set_grammar_rules", "grammar_rules"), &LanguageAssetResource::set_grammar_rules);
    ClassDB::bind_method(D_METHOD("get_grammar_rules"), &LanguageAssetResource::get_grammar_rules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "grammar_rules", PROPERTY_HINT_ARRAY_TYPE, "GrammarRuleResource"),
        "set_grammar_rules",
        "get_grammar_rules"
    );

    ClassDB::bind_method(D_METHOD("add_lexicon", "lexicon"), &LanguageAssetResource::add_lexicon);
    ClassDB::bind_method(D_METHOD("add_grammar_rule", "rule"), &LanguageAssetResource::add_grammar_rule);
    ClassDB::bind_method(
        D_METHOD("populate_builtin_godot_and_wizard_vocabulary"),
        &LanguageAssetResource::populate_builtin_godot_and_wizard_vocabulary
    );

    // External NLP Importers & Exporters
    ClassDB::bind_method(
        D_METHOD("import_cmudict_text", "cmudict_text", "lect_id"),
        &LanguageAssetResource::import_cmudict_text,
        DEFVAL(0)
    );
    ClassDB::bind_method(
        D_METHOD("import_huggingface_tokenizer_json", "json_text"),
        &LanguageAssetResource::import_huggingface_tokenizer_json
    );
    ClassDB::bind_method(
        D_METHOD("import_sentencepiece_vocab_text", "tsv_text"),
        &LanguageAssetResource::import_sentencepiece_vocab_text
    );
    ClassDB::bind_method(
        D_METHOD("import_wordnet_synsets_json", "json_text"),
        &LanguageAssetResource::import_wordnet_synsets_json
    );
    ClassDB::bind_method(
        D_METHOD("import_mrl_embeddings_json", "json_text"),
        &LanguageAssetResource::import_mrl_embeddings_json
    );

    ClassDB::bind_method(D_METHOD("tokenize_bpe_subwords", "text"), &LanguageAssetResource::tokenize_bpe_subwords);
    ClassDB::bind_method(D_METHOD("tokenize_bpe_pieces", "text"), &LanguageAssetResource::tokenize_bpe_pieces);
    ClassDB::bind_method(D_METHOD("decode_bpe_tokens", "token_ids"), &LanguageAssetResource::decode_bpe_tokens);
    ClassDB::bind_method(
        D_METHOD("export_gbnf_grammar", "context_tags", "lect_id", "min_affinity"),
        &LanguageAssetResource::export_gbnf_grammar,
        DEFVAL(0),
        DEFVAL(0.15f)
    );

    ClassDB::bind_method(D_METHOD("decompose_word_morphology", "word"), &LanguageAssetResource::decompose_word_morphology);
    ClassDB::bind_method(
        D_METHOD("lookup_or_infer_sememe_for_token", "token"),
        &LanguageAssetResource::lookup_or_infer_sememe_for_token
    );
    ClassDB::bind_method(
        D_METHOD("find_semantic_frame_for_verb", "verb"),
        &LanguageAssetResource::find_semantic_frame_for_verb
    );
    ClassDB::bind_method(
        D_METHOD("is_sememe_hypernym_of", "hypernym_label", "hyponym_label", "max_depth"),
        &LanguageAssetResource::is_sememe_hypernym_of,
        DEFVAL(6)
    );
    ClassDB::bind_method(
        D_METHOD("encode_text_to_phoneme_masks", "text", "lect_id"),
        &LanguageAssetResource::encode_text_to_phoneme_masks,
        DEFVAL(0)
    );
    ClassDB::bind_method(
        D_METHOD("decode_noisy_phonemes_to_surface", "noisy_masks", "expected_sememe_id", "lect_id"),
        &LanguageAssetResource::decode_noisy_phonemes_to_surface,
        DEFVAL(-1),
        DEFVAL(0)
    );
    ClassDB::bind_method(
        D_METHOD("apply_cascading_grammar", "base_word", "lexical_category", "lect_id", "active_seme_tags"),
        &LanguageAssetResource::apply_cascading_grammar
    );
    ClassDB::bind_method(
        D_METHOD("compile_slm_vocabulary_constraints", "context_tags", "lect_id", "previous_lexical_category", "min_affinity"),
        &LanguageAssetResource::compile_slm_vocabulary_constraints,
        DEFVAL(0),
        DEFVAL(0),
        DEFVAL(0.20f)
    );

    ClassDB::bind_method(D_METHOD("get_snapshot_state"), &LanguageAssetResource::get_snapshot_state);
    ClassDB::bind_method(D_METHOD("apply_snapshot_state", "state"), &LanguageAssetResource::apply_snapshot_state);
}

void LanguageAssetResource::set_lexicons(const godot::TypedArray<LexiconResource> &p_lexicons) {
    lexicons_      = p_lexicons;
    unified_dirty_ = true;
    emit_changed();
}

void LanguageAssetResource::set_grammar_rules(const godot::TypedArray<GrammarRuleResource> &p_rules) {
    grammar_rules_ = p_rules;
    emit_changed();
}

void LanguageAssetResource::add_lexicon(const godot::Ref<LexiconResource> &p_lexicon) {
    if (p_lexicon.is_valid() && !lexicons_.has(p_lexicon)) {
        lexicons_.push_back(p_lexicon);
        unified_dirty_ = true;
        emit_changed();
    }
}

void LanguageAssetResource::add_grammar_rule(const godot::Ref<GrammarRuleResource> &p_rule) {
    if (p_rule.is_valid() && !grammar_rules_.has(p_rule)) {
        grammar_rules_.push_back(p_rule);
        emit_changed();
    }
}

godot::Ref<LexiconResource> LanguageAssetResource::_ensure_primary_lexicon() {
    godot::Ref<LexiconResource> base_lex;
    if (lexicons_.is_empty()) {
        base_lex.instantiate();
        base_lex->set_lexicon_name("IdeamCoreBuiltInLexicon");
        lexicons_.push_back(base_lex);
    } else {
        base_lex = lexicons_[0];
        if (base_lex.is_null()) {
            base_lex.instantiate();
            base_lex->set_lexicon_name("IdeamCoreBuiltInLexicon");
            lexicons_[0] = base_lex;
        }
    }
    return base_lex;
}

void LanguageAssetResource::populate_builtin_godot_and_wizard_vocabulary() {
    godot::Ref<LexiconResource> base_lex = _ensure_primary_lexicon();

    auto add_vocab = [&](const char *p_word,
                         core::UniversalPOS p_cat,
                         core::SemanticDomain p_dom,
                         std::initializer_list<const char *> p_nuclear,
                         std::initializer_list<const char *> p_peripheral,
                         float p_valence = 0.0f,
                         std::initializer_list<const char *> p_hypernyms = {}) {
        godot::Ref<SememeResource> sem;
        sem.instantiate();
        sem->set_canonical_label(godot::StringName(p_word));
        sem->set_upos_tag(static_cast<int>(p_cat));
        sem->set_primary_domain_id(static_cast<int>(p_dom));
        sem->set_emotive_valence(p_valence);
        for (const char *t : p_nuclear) {
            sem->add_nuclear_seme_tag(godot::String(t));
        }
        for (const char *t : p_peripheral) {
            sem->add_peripheral_seme_tag(godot::String(t));
        }
        godot::PackedStringArray hyps;
        for (const char *h : p_hypernyms) {
            hyps.push_back(godot::String(h));
        }
        sem->set_hypernym_labels(hyps);

        base_lex->register_sememe_resource(sem);
        base_lex->register_lexeme_entry(
            godot::String(p_word),
            godot::StringName(p_word),
            static_cast<int>(p_cat),
            default_lect_id_,
            128,
            1.0f,
            static_cast<int>(core::hash_linguistic_token(p_word) & 0x7FFFu)
        );
    };

    // Spatial & Perspective Vocabulary
    add_vocab("top_down",     core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_top_down", "2d"}, {"strategy"});
    add_vocab("first_person", core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_first_person", "3d"}, {"immersive"});
    add_vocab("third_person", core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_third_person", "3d"}, {"action"});
    add_vocab("isometric",    core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_isometric", "3d"}, {"strategy", "rpg"});
    add_vocab("vr",           core::UniversalPOS::NOUN, core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_xr_immersive", "3d", "spatial_first_person"}, {"immersive"});
    add_vocab("xr",           core::UniversalPOS::NOUN, core::SemanticDomain::SPATIAL_TOPOLOGY, {"spatial_xr_immersive", "3d", "spatial_first_person"}, {"immersive"});
    add_vocab("2d",           core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"2d"}, {});
    add_vocab("3d",           core::UniversalPOS::ADJ,  core::SemanticDomain::SPATIAL_TOPOLOGY, {"3d"}, {});

    // Era, Genre & Setting Vocabulary
    add_vocab("medieval",     core::UniversalPOS::ADJ,  core::SemanticDomain::TEMPORAL_ERA,     {"era_medieval"}, {"castle", "sword"});
    add_vocab("ancient",      core::UniversalPOS::ADJ,  core::SemanticDomain::TEMPORAL_ERA,     {"ancient"}, {"ruins"});
    add_vocab("steampunk",    core::UniversalPOS::ADJ,  core::SemanticDomain::TEMPORAL_ERA,     {"industrial", "steampunk"}, {"futuristic"});
    add_vocab("modern",       core::UniversalPOS::ADJ,  core::SemanticDomain::TEMPORAL_ERA,     {"modern"}, {});
    add_vocab("cyberpunk",    core::UniversalPOS::ADJ,  core::SemanticDomain::TEMPORAL_ERA,     {"futuristic", "scifi"}, {"dark"}, -0.15f);
    add_vocab("fantasy",      core::UniversalPOS::NOUN, core::SemanticDomain::GENRE_AND_TONE,   {"genre_fantasy"}, {"era_medieval", "magic"}, 0.25f);
    add_vocab("scifi",        core::UniversalPOS::NOUN, core::SemanticDomain::GENRE_AND_TONE,   {"scifi", "futuristic"}, {"space"});
    add_vocab("gothic",       core::UniversalPOS::ADJ,  core::SemanticDomain::GENRE_AND_TONE,   {"gothic", "horror"}, {"era_medieval", "dark"}, -0.40f);
    add_vocab("horror",       core::UniversalPOS::NOUN, core::SemanticDomain::GENRE_AND_TONE,   {"horror"}, {"dark", "hazard"}, -0.65f);
    add_vocab("mystery",      core::UniversalPOS::NOUN, core::SemanticDomain::GENRE_AND_TONE,   {"mystery"}, {"investigation", "puzzle"}, 0.10f);

    // System & Mechanic Vocabulary
    add_vocab("rpg",          core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"mechanic_rpg"}, {"agent", "character"});
    add_vocab("action",       core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"action", "real_time"}, {"movable"});
    add_vocab("adventure",    core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"action"}, {"mystery", "interactive"});
    add_vocab("strategy",     core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"strategy"}, {"turn_based", "board"});
    add_vocab("tactical",     core::UniversalPOS::ADJ,  core::SemanticDomain::SYSTEM_MECHANIC,  {"strategy"}, {"turn_based", "combat"});
    add_vocab("puzzle",       core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"puzzle"}, {"interactive"});
    add_vocab("simulation",   core::UniversalPOS::NOUN, core::SemanticDomain::SYSTEM_MECHANIC,  {"simulation"}, {"procedural"});
    add_vocab("procedural",   core::UniversalPOS::ADJ,  core::SemanticDomain::SYSTEM_MECHANIC,  {"procedural"}, {"wfc"});

    // Ontology, Architecture & Godot Entities (with WordNet Hypernym Chains)
    add_vocab("agent",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"agent", "movable"}, {"interactive", "communicative"}, 0.0f);
    add_vocab("character",    core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"agent", "character"}, {"communicative", "movable"}, 0.0f, {"agent"});
    add_vocab("player",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"agent", "player"}, {"interactive", "movable"}, 0.30f, {"character"});
    add_vocab("creature",     core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"creature", "movable"}, {"agent"}, 0.0f, {"agent"});
    add_vocab("monster",      core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"creature", "hazard"}, {"combat"}, -0.60f, {"creature"});
    add_vocab("prop",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"prop"}, {"interactive"}, 0.0f);
    add_vocab("item",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"item", "prop"}, {"interactive"}, 0.0f, {"prop"});
    add_vocab("weapon",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"weapon", "item"}, {"combat"}, -0.25f, {"item"});
    add_vocab("sword",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"weapon", "item"}, {"era_medieval", "combat"}, -0.20f, {"weapon"});
    add_vocab("garment",      core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"garment", "item"}, {"character"}, 0.0f, {"item"});
    add_vocab("structure",    core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"structure"}, {"3d"}, 0.0f);
    add_vocab("castle",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"structure", "castle"}, {"era_medieval", "exterior"}, 0.0f, {"structure"});
    add_vocab("tower",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"structure", "vertical", "tower"}, {"castle", "era_medieval"}, 0.0f, {"structure"});
    add_vocab("ruins",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"structure", "exterior"}, {"ancient", "damaged"}, -0.20f, {"structure"});
    add_vocab("grounds",      core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"room", "exterior"}, {"structure"}, 0.0f, {"structure"});
    add_vocab("room",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"room", "interior"}, {"structure"}, 0.0f, {"structure"});
    add_vocab("cell",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"room", "cell"}, {"procedural", "structure"}, 0.0f, {"room"});
    add_vocab("landing",      core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"room", "vertical"}, {"stairs", "structure"}, 0.0f, {"room"});
    add_vocab("stairs",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"vertical", "stairs"}, {"structure", "movable"}, 0.0f, {"structure"});
    add_vocab("aperture",     core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"aperture"}, {"structure"}, 0.0f, {"structure"});
    add_vocab("gate",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"aperture", "gate"}, {"lockable", "structure"}, 0.0f, {"aperture"});
    add_vocab("door",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"aperture", "door"}, {"lockable", "interactive"}, 0.0f, {"aperture"});
    add_vocab("window",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"aperture", "window"}, {"structure", "light"}, 0.0f, {"aperture"});
    add_vocab("chest",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"container", "prop"}, {"lockable", "interactive"}, 0.35f, {"prop"});
    add_vocab("trap",         core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"hazard", "prop"}, {"hidden", "combat"}, -0.55f, {"prop"});
    add_vocab("light",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"light"}, {"AFFECT_POSITIVE"}, 0.25f);
    add_vocab("audio",        core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"audio"}, {"communicative"});
    add_vocab("sensor",       core::UniversalPOS::NOUN, core::SemanticDomain::ENTITY_ONTOLOGY,  {"sensor"}, {"interactive"});

    // Capabilities, Verbs & Reflected Actions
    add_vocab("move",         core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"movable"}, {"action"});
    add_vocab("interact",     core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"interactive"}, {"action"});
    add_vocab("open",         core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"interactive", "aperture"}, {"lockable"});
    add_vocab("lock",         core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"lockable", "locked"}, {"aperture", "container"});
    add_vocab("attack",       core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"combat", "action"}, {"weapon"}, -0.45f);
    add_vocab("speak",        core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"communicative", "audio"}, {"agent"});
    add_vocab("listen",       core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"communicative", "sensor"}, {"agent", "audio"});
    add_vocab("heal",         core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"AFFECT_POSITIVE"}, {"agent"}, 0.65f);
    add_vocab("destroy",      core::UniversalPOS::VERB, core::SemanticDomain::CAPABILITY_ACTION,{"destructible", "damaged"}, {"combat"}, -0.55f);

    // UI & Reflection Archetypes
    add_vocab("label",        core::UniversalPOS::NOUN, core::SemanticDomain::UI_AND_REFLECTION,{"2d"}, {"communicative"});
    add_vocab("button",       core::UniversalPOS::NOUN, core::SemanticDomain::UI_AND_REFLECTION,{"interactive", "2d"}, {"action"});
    add_vocab("edit",         core::UniversalPOS::NOUN, core::SemanticDomain::UI_AND_REFLECTION,{"interactive", "communicative"}, {"2d"});
    add_vocab("health",       core::UniversalPOS::NOUN, core::SemanticDomain::UI_AND_REFLECTION,{"agent", "destructible"}, {"mechanic_rpg"}, 0.20f);
    add_vocab("speed",        core::UniversalPOS::NOUN, core::SemanticDomain::UI_AND_REFLECTION,{"movable"}, {"action"});

    // Built-in FrameNet SemanticFrameResources
    auto add_frame = [&](const char *p_name,
                         std::initializer_list<const char *> p_verbs,
                         std::initializer_list<const char *> p_agent_tags,
                         std::initializer_list<const char *> p_patient_tags,
                         std::initializer_list<const char *> p_instr_tags,
                         std::initializer_list<const char *> p_conseq_tags,
                         float p_valence) {
        godot::Ref<SemanticFrameResource> fr;
        fr.instantiate();
        fr->set_frame_name(godot::StringName(p_name));
        godot::PackedStringArray verbs, ag, pat, ins, con;
        for (const char *v : p_verbs)        verbs.push_back(godot::String(v));
        for (const char *a : p_agent_tags)   ag.push_back(godot::String(a));
        for (const char *p : p_patient_tags) pat.push_back(godot::String(p));
        for (const char *i : p_instr_tags)   ins.push_back(godot::String(i));
        for (const char *c : p_conseq_tags)  con.push_back(godot::String(c));
        fr->set_evoking_lexical_units(verbs);
        fr->set_required_agent_seme_tags(ag);
        fr->set_required_patient_seme_tags(pat);
        fr->set_required_instrument_seme_tags(ins);
        fr->set_consequent_state_seme_tags(con);
        fr->set_valence_polarity(p_valence);
        base_lex->register_semantic_frame_resource(fr);
    };

    add_frame("Attack_combat",         {"attack", "strike", "destroy"}, {"agent"}, {"agent", "destructible"}, {"weapon"}, {"damaged"}, -0.60f);
    add_frame("Unlock_aperture",       {"open", "unlock", "lock"},      {"agent"}, {"aperture", "lockable"},  {"item"},   {"interactive"}, 0.20f);
    add_frame("Motion_directional",    {"move", "walk", "climb"},       {"movable"}, {"structure", "room"},   {},         {"movable"}, 0.10f);
    add_frame("Communicate_discourse", {"speak", "listen", "ask"},      {"agent", "communicative"}, {"agent"}, {"audio"}, {"communicative"}, 0.25f);
    add_frame("Cure_healer",           {"heal", "restore"},             {"agent"}, {"agent"},                 {"item"},   {"AFFECT_POSITIVE"}, 0.75f);

    unified_dirty_ = true;
    _rebuild_unified_core_if_dirty();
    emit_changed();
}

void LanguageAssetResource::_rebuild_unified_core_if_dirty() const {
    if (!unified_dirty_) {
        return;
    }
    unified_core_.clear();
    unified_core_.populate_default_core_semes_and_morphemes();

    for (int i = 0; i < lexicons_.size(); ++i) {
        godot::Ref<LexiconResource> lex = lexicons_[i];
        if (lex.is_valid()) {
            unified_core_.merge_from(lex->get_core_lexicon());
        }
    }
    unified_dirty_ = false;
}

const core::SemanticLexiconCore &LanguageAssetResource::get_unified_core_lexicon() const {
    _rebuild_unified_core_if_dirty();
    return unified_core_;
}

int LanguageAssetResource::import_cmudict_text(const godot::String &p_cmudict_text, int p_lect_id) {
    godot::Ref<LexiconResource> lex = _ensure_primary_lexicon();
    const int count = lex->import_cmudict_text(p_cmudict_text, p_lect_id);
    if (count > 0) {
        unified_dirty_ = true;
        emit_changed();
    }
    return count;
}

int LanguageAssetResource::import_huggingface_tokenizer_json(const godot::String &p_json_text) {
    godot::Ref<LexiconResource> lex = _ensure_primary_lexicon();
    const int count = lex->import_huggingface_tokenizer_json(p_json_text);
    if (count > 0) {
        unified_dirty_ = true;
        emit_changed();
    }
    return count;
}

int LanguageAssetResource::import_sentencepiece_vocab_text(const godot::String &p_tsv_text) {
    godot::Ref<LexiconResource> lex = _ensure_primary_lexicon();
    const int count = lex->import_sentencepiece_vocab_text(p_tsv_text);
    if (count > 0) {
        unified_dirty_ = true;
        emit_changed();
    }
    return count;
}

int LanguageAssetResource::import_wordnet_synsets_json(const godot::String &p_json_text) {
    godot::Ref<LexiconResource> lex = _ensure_primary_lexicon();
    const int count = lex->import_wordnet_synsets_json(p_json_text);
    if (count > 0) {
        unified_dirty_ = true;
        emit_changed();
    }
    return count;
}

int LanguageAssetResource::import_mrl_embeddings_json(const godot::String &p_json_text) {
    godot::Ref<LexiconResource> lex = _ensure_primary_lexicon();
    const int count = lex->import_mrl_embeddings_json(p_json_text);
    if (count > 0) {
        unified_dirty_ = true;
        emit_changed();
    }
    return count;
}

godot::PackedInt32Array LanguageAssetResource::tokenize_bpe_subwords(const godot::String &p_text) const {
    _rebuild_unified_core_if_dirty();
    std::vector<uint32_t> ids;
    unified_core_.tokenize_bpe_subwords(p_text.utf8().get_data(), ids);
    godot::PackedInt32Array out;
    out.resize(static_cast<int64_t>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(ids[i]));
    }
    return out;
}

godot::PackedStringArray LanguageAssetResource::tokenize_bpe_pieces(const godot::String &p_text) const {
    _rebuild_unified_core_if_dirty();
    std::vector<uint32_t> ids;
    unified_core_.tokenize_bpe_subwords(p_text.utf8().get_data(), ids);
    godot::PackedStringArray out;
    for (uint32_t id : ids) {
        std::string_view piece = unified_core_.get_bpe_token_piece(id);
        out.push_back(godot::String::utf8(piece.data(), static_cast<int>(piece.size())));
    }
    return out;
}

godot::String LanguageAssetResource::decode_bpe_tokens(const godot::PackedInt32Array &p_token_ids) const {
    _rebuild_unified_core_if_dirty();
    std::vector<uint32_t> ids(static_cast<size_t>(p_token_ids.size()));
    for (int64_t i = 0; i < p_token_ids.size(); ++i) {
        ids[static_cast<size_t>(i)] = static_cast<uint32_t>(std::max<int32_t>(0, p_token_ids[i]));
    }
    const std::string decoded = unified_core_.decode_bpe_tokens(ids);
    return godot::String::utf8(decoded.c_str(), static_cast<int>(decoded.size()));
}

godot::String LanguageAssetResource::export_gbnf_grammar(
    const godot::Ref<SemanticTagSetResource> &p_context_tags,
    int p_lect_id,
    float p_min_affinity
) const {
    _rebuild_unified_core_if_dirty();
    const core::SememeDescriptorPOD ctx_desc =
        p_context_tags.is_valid() ? p_context_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};
    std::string gbnf = unified_core_.export_gbnf_grammar_for_constraints(
        ctx_desc,
        static_cast<uint16_t>(std::max(0, p_lect_id)),
        0xFFFFu,
        p_min_affinity
    );

    // Append any custom GBNF productions from cascading GrammarRuleResources
    const godot::PackedStringArray active_tags =
        p_context_tags.is_valid() ? p_context_tags->get_all_nuclear_tags() : godot::PackedStringArray();
    for (int i = 0; i < grammar_rules_.size(); ++i) {
        godot::Ref<GrammarRuleResource> r = grammar_rules_[i];
        if (r.is_valid() &&
            r->get_operation() == GRAMMAR_OP_EMIT_GBNF_PRODUCTION &&
            !r->get_gbnf_production_pattern().is_empty() &&
            r->matches_context(0, p_lect_id, active_tags)) {
            gbnf += "\n";
            gbnf += r->get_gbnf_production_pattern().utf8().get_data();
        }
    }
    return godot::String::utf8(gbnf.c_str(), static_cast<int>(gbnf.size()));
}

godot::Dictionary LanguageAssetResource::decompose_word_morphology(const godot::String &p_word) const {
    _rebuild_unified_core_if_dirty();
    const auto res = unified_core_.decompose_word_morphemes(p_word.utf8().get_data());
    godot::Dictionary d;
    d["matched_lexeme_id"]   = (res.matched_lexeme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.matched_lexeme_id);
    d["matched_sememe_id"]   = (res.matched_sememe_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.matched_sememe_id);
    d["prefix_morpheme_id"]  = (res.prefix_morpheme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.prefix_morpheme_id);
    d["suffix_morpheme_id"]  = (res.suffix_morpheme_id == 0xFFFFFFFFu) ? -1 : static_cast<int>(res.suffix_morpheme_id);
    d["inferred_category"]   = static_cast<int>(res.inferred_category);
    d["upos_name"]           = godot::String(core::upos_to_string(res.inferred_category).data());
    d["is_negated"]          = res.is_negated;
    d["nuclear_popcount"]    = static_cast<int>(res.composed_descriptor.nuclear_popcount());
    d["peripheral_popcount"] = static_cast<int>(res.composed_descriptor.peripheral_popcount());
    return d;
}

godot::Ref<SememeResource> LanguageAssetResource::lookup_or_infer_sememe_for_token(
    const godot::String &p_token
) const {
    _rebuild_unified_core_if_dirty();
    const godot::StringName label_sn(p_token.to_lower());
    for (int i = 0; i < lexicons_.size(); ++i) {
        godot::Ref<LexiconResource> lex = lexicons_[i];
        if (lex.is_valid()) {
            godot::Ref<SememeResource> found = lex->find_sememe_resource_by_label(label_sn);
            if (found.is_valid()) {
                return found;
            }
        }
    }

    const auto morph = unified_core_.decompose_word_morphemes(p_token.utf8().get_data());
    godot::Ref<SememeResource> inferred;
    inferred.instantiate();
    inferred->set_canonical_label(label_sn);
    inferred->set_upos_tag(static_cast<int>(morph.inferred_category));
    inferred->set_emotive_valence(morph.composed_descriptor.emotive_valence);

    const auto &seme_names = unified_core_.seme_names();
    const auto &seme_pods  = unified_core_.semes();
    for (size_t i = 0; i < seme_pods.size() && i < seme_names.size(); ++i) {
        const uint32_t bit = seme_pods[i].seme_id;
        if (morph.composed_descriptor.test_nuclear_bit(bit)) {
            inferred->add_nuclear_seme_tag(godot::String::utf8(seme_names[i].c_str()));
        } else if (morph.composed_descriptor.test_peripheral_bit(bit)) {
            inferred->add_peripheral_seme_tag(godot::String::utf8(seme_names[i].c_str()));
        }
    }
    if (inferred->get_nuclear_seme_tags().is_empty() && !p_token.is_empty()) {
        inferred->add_nuclear_seme_tag(p_token.to_lower());
    }
    return inferred;
}

godot::Ref<SemanticFrameResource> LanguageAssetResource::find_semantic_frame_for_verb(
    const godot::String &p_verb
) const {
    const godot::String lower_v = p_verb.to_lower();
    for (int i = 0; i < lexicons_.size(); ++i) {
        godot::Ref<LexiconResource> lex = lexicons_[i];
        if (lex.is_null()) continue;
        const godot::TypedArray<SemanticFrameResource> frames = lex->get_semantic_frame_resources();
        for (int j = 0; j < frames.size(); ++j) {
            godot::Ref<SemanticFrameResource> fr = frames[j];
            if (fr.is_valid() && fr->get_evoking_lexical_units().has(lower_v)) {
                return fr;
            }
        }
    }
    return {};
}

bool LanguageAssetResource::is_sememe_hypernym_of(
    const godot::StringName &p_hypernym_label,
    const godot::StringName &p_hyponym_label,
    int p_max_depth
) const {
    _rebuild_unified_core_if_dirty();
    const int32_t hyp_id = unified_core_.find_sememe_by_label(godot::String(p_hypernym_label).utf8().get_data());
    const int32_t sub_id = unified_core_.find_sememe_by_label(godot::String(p_hyponym_label).utf8().get_data());
    if (hyp_id < 0 || sub_id < 0) return false;
    return unified_core_.is_hypernym_of(
        static_cast<uint32_t>(hyp_id),
        static_cast<uint32_t>(sub_id),
        static_cast<uint32_t>(std::max(1, p_max_depth))
    );
}

godot::PackedInt32Array LanguageAssetResource::encode_text_to_phoneme_masks(
    const godot::String &p_text,
    int p_lect_id
) const {
    _rebuild_unified_core_if_dirty();
    godot::PackedInt32Array out;
    const godot::PackedStringArray words = p_text.split(" ", false);
    for (int i = 0; i < words.size(); ++i) {
        std::vector<uint16_t> masks;
        const godot::CharString cs = words[i].utf8();
        const int32_t lx_id = unified_core_.find_lexeme_by_surface(
            cs.get_data(),
            static_cast<uint16_t>(std::max(0, p_lect_id))
        );
        if (lx_id >= 0) {
            std::span<const uint16_t> slice = unified_core_.get_lexeme_phoneme_masks(static_cast<uint32_t>(lx_id));
            masks.assign(slice.begin(), slice.end());
        } else {
            unified_core_.approximate_phonemes_from_text(cs.get_data(), masks);
        }
        for (uint16_t m : masks) {
            out.push_back(static_cast<int32_t>(m));
        }
    }
    return out;
}

godot::String LanguageAssetResource::decode_noisy_phonemes_to_surface(
    const godot::PackedInt32Array &p_noisy_masks,
    int p_expected_sememe_id,
    int p_lect_id
) const {
    _rebuild_unified_core_if_dirty();
    std::vector<uint16_t> masks(static_cast<size_t>(p_noisy_masks.size()));
    for (int64_t i = 0; i < p_noisy_masks.size(); ++i) {
        masks[static_cast<size_t>(i)] = static_cast<uint16_t>(p_noisy_masks[i] & 0xFFFF);
    }
    const uint32_t best_lx = unified_core_.find_best_matching_lexeme_from_noisy_phonemes(
        masks,
        (p_expected_sememe_id >= 0) ? static_cast<uint32_t>(p_expected_sememe_id) : 0xFFFFFFFFu,
        static_cast<uint16_t>(std::max(0, p_lect_id))
    );
    if (best_lx == 0xFFFFFFFFu) {
        return godot::String("...");
    }
    std::string_view surf = unified_core_.get_lexeme_surface(best_lx);
    return godot::String::utf8(surf.data(), static_cast<int>(surf.size()));
}

godot::String LanguageAssetResource::apply_cascading_grammar(
    const godot::String &p_base_word,
    int p_lexical_category,
    int p_lect_id,
    const godot::PackedStringArray &p_active_seme_tags
) const {
    std::vector<godot::Ref<GrammarRuleResource>> matching_rules;
    for (int i = 0; i < grammar_rules_.size(); ++i) {
        godot::Ref<GrammarRuleResource> root_rule = grammar_rules_[i];
        if (root_rule.is_null()) continue;
        godot::TypedArray<GrammarRuleResource> cascaded = root_rule->collect_cascaded_rules();
        for (int j = 0; j < cascaded.size(); ++j) {
            godot::Ref<GrammarRuleResource> r = cascaded[j];
            if (r.is_valid() && r->matches_context(p_lexical_category, p_lect_id, p_active_seme_tags, p_base_word)) {
                matching_rules.push_back(r);
            }
        }
    }

    std::stable_sort(
        matching_rules.begin(),
        matching_rules.end(),
        [](const godot::Ref<GrammarRuleResource> &a, const godot::Ref<GrammarRuleResource> &b) {
            return !a->outweighs(b);
        }
    );

    godot::String result = p_base_word;
    for (const auto &r : matching_rules) {
        result = r->apply_to_surface_word(result);
    }
    return result;
}

godot::Dictionary LanguageAssetResource::compile_slm_vocabulary_constraints(
    const godot::Ref<SemanticTagSetResource> &p_context_tags,
    int p_lect_id,
    int p_previous_lexical_category,
    float p_min_affinity
) const {
    _rebuild_unified_core_if_dirty();
    uint16_t allowed_cat_mask = 0xFFFFu;
    const godot::PackedStringArray active_tags =
        p_context_tags.is_valid() ? p_context_tags->get_all_nuclear_tags() : godot::PackedStringArray();

    for (int i = 0; i < grammar_rules_.size(); ++i) {
        godot::Ref<GrammarRuleResource> r = grammar_rules_[i];
        if (r.is_valid() &&
            r->get_operation() == GRAMMAR_OP_CONSTRAIN_SLM_NEXT_CATEGORY &&
            r->matches_context(p_previous_lexical_category, p_lect_id, active_tags)) {
            allowed_cat_mask &= static_cast<uint16_t>(r->get_slm_allowed_category_mask());
        }
    }

    const core::SememeDescriptorPOD ctx_desc =
        p_context_tags.is_valid() ? p_context_tags->get_compiled_descriptor() : core::SememeDescriptorPOD{};

    std::vector<uint32_t> bpe_tokens;
    std::vector<uint32_t> lexeme_ids;
    unified_core_.build_slm_vocabulary_mask(
        ctx_desc,
        static_cast<uint16_t>(std::max(0, p_lect_id)),
        allowed_cat_mask,
        p_min_affinity,
        bpe_tokens,
        lexeme_ids
    );

    godot::PackedInt32Array bpe_arr;
    bpe_arr.resize(static_cast<int64_t>(bpe_tokens.size()));
    for (size_t i = 0; i < bpe_tokens.size(); ++i) {
        bpe_arr.set(static_cast<int64_t>(i), static_cast<int32_t>(bpe_tokens[i]));
    }

    godot::PackedStringArray surface_words;
    for (uint32_t lx_id : lexeme_ids) {
        std::string_view surf = unified_core_.get_lexeme_surface(lx_id);
        if (!surf.empty()) {
            surface_words.push_back(godot::String::utf8(surf.data(), static_cast<int>(surf.size())));
        }
    }

    godot::Dictionary out;
    out["lect_id"]                = p_lect_id;
    out["allowed_category_mask"]  = static_cast<int>(allowed_cat_mask);
    out["allowed_bpe_token_ids"]  = bpe_arr;
    out["allowed_surface_words"]  = surface_words;
    out["gbnf_grammar"]           = export_gbnf_grammar(p_context_tags, p_lect_id, p_min_affinity);
    out["coherence_trait_mask32"] = static_cast<int64_t>(ctx_desc.compute_coherence_trait_mask32());
    return out;
}

godot::Dictionary LanguageAssetResource::get_snapshot_state() const {
    godot::Dictionary d;
    d["language_name"]   = language_name_;
    d["language_code"]   = language_code_;
    d["default_lect_id"] = default_lect_id_;
    d["lexicons"]        = lexicons_.duplicate();
    d["grammar_rules"]   = grammar_rules_.duplicate();
    return d;
}

void LanguageAssetResource::apply_snapshot_state(const godot::Dictionary &p_state) {
    if (p_state.has("language_name"))   language_name_   = p_state["language_name"];
    if (p_state.has("language_code"))   language_code_   = p_state["language_code"];
    if (p_state.has("default_lect_id")) default_lect_id_ = static_cast<int>(p_state["default_lect_id"]);
    if (p_state.has("lexicons"))        lexicons_        = p_state["lexicons"];
    if (p_state.has("grammar_rules"))   grammar_rules_   = p_state["grammar_rules"];
    unified_dirty_ = true;
    emit_changed();
}

void LanguageAssetResource::commit_snapshot_mutation(
    const godot::String &p_action_name,
    const godot::Dictionary &p_before_state
) {
    if (undo_redo_.is_null()) return;
    const godot::Dictionary after_state = get_snapshot_state();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(godot::Callable(this, "apply_snapshot_state").bind(after_state));
    undo_redo_->add_undo_method(godot::Callable(this, "apply_snapshot_state").bind(p_before_state));
    undo_redo_->commit_action(false);
}

} // namespace ideam::godot_ext
