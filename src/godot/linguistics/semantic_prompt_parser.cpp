#include "semantic_prompt_parser.h"
#include <algorithm>
#include <cctype>
#include <string>

namespace ideam::godot_ext {

namespace {

[[nodiscard]] bool is_stop_word(const godot::String &p_word) {
    return p_word == "a" || p_word == "an" || p_word == "the" ||
           p_word == "of" || p_word == "in" || p_word == "on" ||
           p_word == "at" || p_word == "to" || p_word == "for" ||
           p_word == "with" || p_word == "and" || p_word == "or" ||
           p_word == "by" || p_word == "from" || p_word == "into" ||
           p_word == "is" || p_word == "are" || p_word == "be" ||
           p_word == "as" || p_word == "that" || p_word == "this" ||
           p_word == "game" || p_word == "project";
}

[[nodiscard]] bool is_negation_cue(const godot::String &p_word) {
    return p_word == "no" || p_word == "not" || p_word == "without" ||
           p_word == "non" || p_word == "never" || p_word == "exclude" ||
           p_word == "excluding" || p_word == "forbid";
}

[[nodiscard]] bool is_diminisher_cue(const godot::String &p_word) {
    return p_word == "slightly" || p_word == "subtle" || p_word == "subtly" ||
           p_word == "minor" || p_word == "light" || p_word == "optional" ||
           p_word == "touch";
}

[[nodiscard]] bool is_instrument_preposition(const godot::String &p_word) {
    return p_word == "with" || p_word == "using" || p_word == "via" || p_word == "by";
}

[[nodiscard]] bool is_location_preposition(const godot::String &p_word) {
    return p_word == "in" || p_word == "at" || p_word == "into" || p_word == "inside" ||
           p_word == "on" || p_word == "toward" || p_word == "to" || p_word == "from";
}

} // namespace

void SemanticPromptParser::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_language_asset", "language_asset"), &SemanticPromptParser::set_language_asset);
    ClassDB::bind_method(D_METHOD("get_language_asset"), &SemanticPromptParser::get_language_asset);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "language_asset", PROPERTY_HINT_RESOURCE_TYPE, "LanguageAssetResource"),
        "set_language_asset",
        "get_language_asset"
    );

    ClassDB::bind_method(
        D_METHOD("set_treat_modifiers_as_peripheral", "treat_as_peripheral"),
        &SemanticPromptParser::set_treat_modifiers_as_peripheral
    );
    ClassDB::bind_method(
        D_METHOD("get_treat_modifiers_as_peripheral"),
        &SemanticPromptParser::get_treat_modifiers_as_peripheral
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "treat_modifiers_as_peripheral"),
        "set_treat_modifiers_as_peripheral",
        "get_treat_modifiers_as_peripheral"
    );

    ClassDB::bind_static_method(
        "SemanticPromptParser",
        D_METHOD("split_compound_identifier", "identifier"),
        &SemanticPromptParser::split_compound_identifier
    );

    ClassDB::bind_method(
        D_METHOD("tokenize_and_normalize_prompt", "prompt"),
        &SemanticPromptParser::tokenize_and_normalize_prompt
    );
    ClassDB::bind_method(
        D_METHOD("extract_structured_tokens", "prompt"),
        &SemanticPromptParser::extract_structured_tokens
    );
    ClassDB::bind_method(
        D_METHOD("parse_prompt_to_tag_set", "prompt"),
        &SemanticPromptParser::parse_prompt_to_tag_set
    );
    ClassDB::bind_method(
        D_METHOD("parse_identifier_to_tag_set", "identifier", "head_noun_only_nuclear"),
        &SemanticPromptParser::parse_identifier_to_tag_set,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("parse_prompt_to_semantic_frame", "prompt"),
        &SemanticPromptParser::parse_prompt_to_semantic_frame
    );
    ClassDB::bind_method(
        D_METHOD("score_prompt_similarity", "prompt_a", "prompt_b"),
        &SemanticPromptParser::score_prompt_similarity
    );
    ClassDB::bind_method(
        D_METHOD("compile_prompt_to_slm_constraints", "prompt", "lect_id"),
        &SemanticPromptParser::compile_prompt_to_slm_constraints,
        DEFVAL(0)
    );
}

void SemanticPromptParser::_ensure_default_language_asset() const {
    if (language_asset_.is_null()) {
        language_asset_.instantiate();
        language_asset_->populate_builtin_godot_and_wizard_vocabulary();
    }
}

godot::Ref<LanguageAssetResource> SemanticPromptParser::get_language_asset() const {
    _ensure_default_language_asset();
    return language_asset_;
}

godot::PackedStringArray SemanticPromptParser::split_compound_identifier(
    const godot::String &p_identifier
) {
    godot::PackedStringArray out;
    if (p_identifier.is_empty()) {
        return out;
    }

    const godot::CharString utf8 = p_identifier.utf8();
    const std::string_view sv(utf8.get_data(), static_cast<size_t>(utf8.length()));
    std::string current;

    auto flush_token = [&]() {
        if (!current.empty()) {
            out.push_back(godot::String::utf8(current.c_str(), static_cast<int>(current.size())));
            current.clear();
        }
    };

    for (size_t i = 0; i < sv.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(sv[i]);

        if (c == '_' || c == '-' || c == ' ' || c == '.' || c == '/' || c == ':') {
            flush_token();
            continue;
        }

        if ((c == '1' || c == '2' || c == '3' || c == '4') &&
            (i + 1 < sv.size()) &&
            (sv[i + 1] == 'D' || sv[i + 1] == 'd')) {
            flush_token();
            current.push_back(static_cast<char>(c));
            current.push_back('d');
            flush_token();
            ++i;
            continue;
        }

        if (std::isupper(c)) {
            const bool prev_is_lower = (i > 0) && std::islower(static_cast<unsigned char>(sv[i - 1]));
            const bool prev_is_upper = (i > 0) && std::isupper(static_cast<unsigned char>(sv[i - 1]));
            const bool next_is_lower = (i + 1 < sv.size()) && std::islower(static_cast<unsigned char>(sv[i + 1]));

            if (!current.empty() && (prev_is_lower || (prev_is_upper && next_is_lower))) {
                flush_token();
            }
            current.push_back(static_cast<char>(std::tolower(c)));
        } else if (std::isalnum(c)) {
            current.push_back(static_cast<char>(std::tolower(c)));
        } else {
            flush_token();
        }
    }
    flush_token();
    return out;
}

godot::PackedStringArray SemanticPromptParser::tokenize_and_normalize_prompt(
    const godot::String &p_prompt
) const {
    godot::String normalized = p_prompt.to_lower();

    normalized = normalized.replacen("top-down", "top_down");
    normalized = normalized.replacen("top down", "top_down");
    normalized = normalized.replacen("first-person", "first_person");
    normalized = normalized.replacen("first person", "first_person");
    normalized = normalized.replacen("third-person", "third_person");
    normalized = normalized.replacen("third person", "third_person");
    normalized = normalized.replacen("turn-based", "turn_based");
    normalized = normalized.replacen("turn based", "turn_based");
    normalized = normalized.replacen("real-time", "real_time");
    normalized = normalized.replacen("real time", "real_time");
    normalized = normalized.replacen("side-scroller", "side_scroller");
    normalized = normalized.replacen("side scroller", "side_scroller");
    normalized = normalized.replacen("side-view", "side_view");
    normalized = normalized.replacen("side view", "side_view");
    normalized = normalized.replacen("sci-fi", "scifi");
    normalized = normalized.replacen("sci fi", "scifi");
    normalized = normalized.replacen("science fiction", "scifi");
    normalized = normalized.replacen("role-playing", "rpg");
    normalized = normalized.replacen("role playing", "rpg");
    normalized = normalized.replacen("wave function collapse", "wfc");

    const char *punct = ",.;:!?()[]{}\"'/\\|+-*=";
    for (int i = 0; punct[i] != '\0'; ++i) {
        char buf[2] = {punct[i], '\0'};
        normalized = normalized.replace(buf, " ");
    }

    godot::PackedStringArray raw_words = normalized.split(" ", false);
    godot::PackedStringArray out;
    for (int i = 0; i < raw_words.size(); ++i) {
        godot::String w = raw_words[i].strip_edges();
        if (!w.is_empty()) {
            out.push_back(w);
        }
    }
    return out;
}

godot::Array SemanticPromptParser::extract_structured_tokens(
    const godot::String &p_prompt
) const {
    _ensure_default_language_asset();
    godot::Array structured;
    const godot::PackedStringArray tokens = tokenize_and_normalize_prompt(p_prompt);

    bool negate_next   = false;
    bool diminish_next = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const godot::String tok = tokens[i];
        if (is_negation_cue(tok)) {
            negate_next = true;
            continue;
        }
        if (is_diminisher_cue(tok)) {
            diminish_next = true;
            continue;
        }
        if (is_stop_word(tok)) {
            continue;
        }

        const godot::Dictionary morph  = language_asset_->decompose_word_morphology(tok);
        godot::Ref<SememeResource> sem = language_asset_->lookup_or_infer_sememe_for_token(tok);

        const bool final_negated = negate_next || static_cast<bool>(morph.get("is_negated", false));
        const int upos_val       = sem.is_valid()
            ? sem->get_lexical_category()
            : static_cast<int>(morph.get("inferred_category", 0));

        godot::Dictionary entry;
        entry["surface"]          = tok;
        entry["is_negated"]       = final_negated;
        entry["is_diminished"]    = diminish_next;
        entry["lexical_category"] = upos_val;
        entry["upos_tag"]         = godot::String(core::upos_to_string(static_cast<core::UniversalPOS>(upos_val)).data());
        entry["nuclear_semes"]    = sem.is_valid() ? sem->get_nuclear_seme_tags() : godot::PackedStringArray();
        entry["peripheral_semes"] = sem.is_valid() ? sem->get_peripheral_seme_tags() : godot::PackedStringArray();
        entry["valence"]          = sem.is_valid() ? sem->get_emotive_valence() : 0.0f;
        structured.push_back(entry);

        negate_next   = false;
        diminish_next = false;
    }

    return structured;
}

godot::Ref<SemanticTagSetResource> SemanticPromptParser::parse_prompt_to_tag_set(
    const godot::String &p_prompt
) const {
    _ensure_default_language_asset();
    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();
    tag_set->set_source_prompt(p_prompt);

    if (p_prompt.strip_edges().is_empty()) {
        return tag_set;
    }

    const godot::PackedStringArray tokens = tokenize_and_normalize_prompt(p_prompt);
    bool negate_next   = false;
    bool diminish_next = false;
    float valence_sum  = 0.0f;
    int valence_count  = 0;

    for (int i = 0; i < tokens.size(); ++i) {
        const godot::String tok = tokens[i];
        if (is_negation_cue(tok)) {
            negate_next = true;
            continue;
        }
        if (is_diminisher_cue(tok)) {
            diminish_next = true;
            continue;
        }
        if (is_stop_word(tok)) {
            continue;
        }

        const godot::Dictionary morph  = language_asset_->decompose_word_morphology(tok);
        godot::Ref<SememeResource> sem = language_asset_->lookup_or_infer_sememe_for_token(tok);
        const bool is_negated          = negate_next || static_cast<bool>(morph.get("is_negated", false));

        if (sem.is_valid()) {
            const godot::PackedStringArray nuc = sem->get_nuclear_seme_tags();
            const godot::PackedStringArray per = sem->get_peripheral_seme_tags();

            if (is_negated) {
                for (int j = 0; j < nuc.size(); ++j) {
                    tag_set->add_excluded_tag(nuc[j]);
                }
                valence_sum -= sem->get_emotive_valence();
                ++valence_count;
            } else if (diminish_next || (treat_modifiers_as_peripheral_ &&
                                         sem->get_lexical_category() == static_cast<int>(core::UniversalPOS::ADJ))) {
                for (int j = 0; j < nuc.size(); ++j) {
                    tag_set->add_peripheral_tag(nuc[j]);
                }
                for (int j = 0; j < per.size(); ++j) {
                    tag_set->add_peripheral_tag(per[j]);
                }
                valence_sum += 0.5f * sem->get_emotive_valence();
                ++valence_count;
            } else {
                tag_set->add_sememe(sem);
                for (int j = 0; j < nuc.size(); ++j) {
                    tag_set->add_nuclear_tag(nuc[j]);
                }
                for (int j = 0; j < per.size(); ++j) {
                    tag_set->add_peripheral_tag(per[j]);
                }
                valence_sum += sem->get_emotive_valence();
                ++valence_count;
            }
        }

        negate_next   = false;
        diminish_next = false;
    }

    if (valence_count > 0) {
        tag_set->set_aggregate_valence(
            std::clamp(valence_sum / static_cast<float>(valence_count), -1.0f, 1.0f)
        );
    }

    return tag_set;
}

godot::Ref<SemanticTagSetResource> SemanticPromptParser::parse_identifier_to_tag_set(
    const godot::String &p_identifier,
    bool p_head_noun_only_nuclear
) const {
    _ensure_default_language_asset();
    godot::Ref<SemanticTagSetResource> tag_set;
    tag_set.instantiate();
    tag_set->set_source_prompt(p_identifier);

    const godot::PackedStringArray parts = split_compound_identifier(p_identifier);
    if (parts.is_empty()) {
        return tag_set;
    }

    int head_idx = static_cast<int>(parts.size()) - 1;
    while (head_idx > 0 && (parts[head_idx] == "2d" || parts[head_idx] == "3d" ||
                            parts[head_idx] == "4d" || parts[head_idx] == "1d" ||
                            parts[head_idx] == "node" || parts[head_idx] == "resource")) {
        --head_idx;
    }

    for (int i = 0; i < parts.size(); ++i) {
        const godot::String sub = parts[i];
        if (sub.is_empty() || is_stop_word(sub)) {
            continue;
        }
        godot::Ref<SememeResource> sem = language_asset_->lookup_or_infer_sememe_for_token(sub);
        if (sem.is_null()) {
            continue;
        }

        const bool is_dim_or_head = (i == head_idx) ||
                                    (sub == "1d" || sub == "2d" || sub == "3d" || sub == "4d" ||
                                     sub == "xr" || sub == "vr" || sub == "cell");

        const godot::PackedStringArray nuc = sem->get_nuclear_seme_tags();
        const godot::PackedStringArray per = sem->get_peripheral_seme_tags();

        if (!p_head_noun_only_nuclear || is_dim_or_head) {
            tag_set->add_sememe(sem);
            for (int j = 0; j < nuc.size(); ++j) {
                tag_set->add_nuclear_tag(nuc[j]);
            }
            for (int j = 0; j < per.size(); ++j) {
                tag_set->add_peripheral_tag(per[j]);
            }
        } else {
            for (int j = 0; j < nuc.size(); ++j) {
                tag_set->add_peripheral_tag(nuc[j]);
            }
            for (int j = 0; j < per.size(); ++j) {
                tag_set->add_peripheral_tag(per[j]);
            }
        }
    }

    return tag_set;
}

godot::Dictionary SemanticPromptParser::parse_prompt_to_semantic_frame(
    const godot::String &p_prompt
) const {
    _ensure_default_language_asset();
    const godot::PackedStringArray tokens = tokenize_and_normalize_prompt(p_prompt);

    godot::String agent_tok;
    godot::String verb_tok;
    godot::String patient_tok;
    godot::String instrument_tok;
    godot::String location_tok;

    bool seen_verb          = false;
    bool next_is_instrument = false;
    bool next_is_location   = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const godot::String tok = tokens[i];
        if (tok == "a" || tok == "an" || tok == "the") {
            continue;
        }
        if (is_instrument_preposition(tok)) {
            next_is_instrument = true;
            next_is_location   = false;
            continue;
        }
        if (is_location_preposition(tok)) {
            next_is_location   = true;
            next_is_instrument = false;
            continue;
        }

        godot::Ref<SememeResource> sem = language_asset_->lookup_or_infer_sememe_for_token(tok);
        const int upos = sem.is_valid() ? sem->get_lexical_category() : static_cast<int>(core::UniversalPOS::NOUN);

        if (upos == static_cast<int>(core::UniversalPOS::VERB) && !seen_verb) {
            verb_tok  = tok;
            seen_verb = true;
            continue;
        }

        if (upos == static_cast<int>(core::UniversalPOS::NOUN) ||
            upos == static_cast<int>(core::UniversalPOS::PROPN) ||
            upos == static_cast<int>(core::UniversalPOS::PRON)) {
            if (next_is_instrument && instrument_tok.is_empty()) {
                instrument_tok     = tok;
                next_is_instrument = false;
            } else if (next_is_location && location_tok.is_empty()) {
                location_tok     = tok;
                next_is_location = false;
            } else if (!seen_verb && agent_tok.is_empty()) {
                agent_tok = tok;
            } else if (seen_verb && patient_tok.is_empty()) {
                patient_tok = tok;
            } else if (agent_tok.is_empty()) {
                agent_tok = tok;
            }
        }
    }

    // Imperative fallback: if prompt starts with a verb ("open door with key"), implicit agent is "player"
    if (agent_tok.is_empty() && !verb_tok.is_empty()) {
        agent_tok = "player";
    }

    godot::Ref<SemanticTagSetResource> agent_tags      = parse_prompt_to_tag_set(agent_tok);
    godot::Ref<SemanticTagSetResource> patient_tags    = parse_prompt_to_tag_set(patient_tok);
    godot::Ref<SemanticTagSetResource> instrument_tags = parse_prompt_to_tag_set(instrument_tok);
    godot::Ref<SemanticFrameResource> matched_frame    = language_asset_->find_semantic_frame_for_verb(verb_tok);

    float role_fit = 0.0f;
    godot::String frame_name = "";
    if (matched_frame.is_valid()) {
        frame_name = godot::String(matched_frame->get_frame_name());
        role_fit   = matched_frame->evaluate_role_fit(agent_tags, patient_tags, instrument_tags);
    }

    godot::Dictionary out;
    out["frame_name"]       = frame_name;
    out["frame_resource"]   = matched_frame;
    out["evoking_verb"]     = verb_tok;
    out["agent_token"]      = agent_tok;
    out["patient_token"]    = patient_tok;
    out["instrument_token"] = instrument_tok;
    out["location_token"]   = location_tok;
    out["role_fit_score"]   = role_fit;
    return out;
}

float SemanticPromptParser::score_prompt_similarity(
    const godot::String &p_prompt_a,
    const godot::String &p_prompt_b
) const {
    godot::Ref<SemanticTagSetResource> set_a = parse_prompt_to_tag_set(p_prompt_a);
    godot::Ref<SemanticTagSetResource> set_b = parse_prompt_to_tag_set(p_prompt_b);
    return set_a->compute_affinity_with(set_b);
}

godot::Dictionary SemanticPromptParser::compile_prompt_to_slm_constraints(
    const godot::String &p_prompt,
    int p_lect_id
) const {
    _ensure_default_language_asset();
    godot::Ref<SemanticTagSetResource> tags = parse_prompt_to_tag_set(p_prompt);
    return language_asset_->compile_slm_vocabulary_constraints(tags, p_lect_id, 0, 0.15f);
}

} // namespace ideam::godot_ext
