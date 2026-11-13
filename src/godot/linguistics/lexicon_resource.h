#pragma once

/**
 * lexicon_resource.h
 * `LexiconResource`, `GrammarRuleResource`, and `LanguageAssetResource`
 * (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Godot-first, DOD-backed successor to `UnityLanguageAsset.cs`, `UnityLexicon`,
 * `UnityGrammar`, and `UnityLanguageLect`.
 *   - `GrammarRuleResource`: Uses the same CSS-like cascading selector & specificity
 *     paradigm as `GameplayRule` and `GenerationRule` so syntax, inflection, and
 *     dialect (`Lect`) transformations cascade cleanly.
 *   - `LanguageAssetResource`: Owns the `core::SemanticLexiconCore` tables for
 *     phonemes, morphemes, lexemes, lects, and sememes, and provides optional
 *     vocabulary constraints for local Small Language Models (SLMs).
 */

#include "sememe_resource.h"
#include "../../core/linguistics/semantic_lexicon_core.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

class GrammarRuleResource : public godot::Resource {
    GDCLASS(GrammarRuleResource, godot::Resource)

private:
    godot::String rule_name_;
    int priority_ = 0;

protected:
    static void _bind_methods();

public:
    GrammarRuleResource() = default;
    ~GrammarRuleResource() override = default;

    void set_rule_name(const godot::String &p_name) { rule_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_rule_name() const { return rule_name_; }

    void set_priority(int p_prio) { priority_ = p_prio; emit_changed(); }
    [[nodiscard]] int get_priority() const noexcept { return priority_; }
};

class LexiconResource : public godot::Resource {
    GDCLASS(LexiconResource, godot::Resource)

private:
    godot::String lexicon_name_;
    core::SemanticLexiconCore core_lexicon_;

protected:
    static void _bind_methods();

public:
    LexiconResource() = default;
    ~LexiconResource() override = default;

    void set_lexicon_name(const godot::String &p_name) { lexicon_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_lexicon_name() const { return lexicon_name_; }

    [[nodiscard]] const core::SemanticLexiconCore &get_core_lexicon() const noexcept { return core_lexicon_; }
};

class LanguageAssetResource : public godot::Resource {
    GDCLASS(LanguageAssetResource, godot::Resource)

private:
    godot::String language_name_ = "Common";
    godot::TypedArray<LexiconResource> lexicons_;
    godot::TypedArray<GrammarRuleResource> grammar_rules_;

protected:
    static void _bind_methods();

public:
    LanguageAssetResource() = default;
    ~LanguageAssetResource() override = default;

    void set_language_name(const godot::String &p_name) { language_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_language_name() const { return language_name_; }

    void set_lexicons(const godot::TypedArray<LexiconResource> &p_lexicons) { lexicons_ = p_lexicons; emit_changed(); }
    [[nodiscard]] godot::TypedArray<LexiconResource> get_lexicons() const { return lexicons_; }

    void set_grammar_rules(const godot::TypedArray<GrammarRuleResource> &p_rules) { grammar_rules_ = p_rules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<GrammarRuleResource> get_grammar_rules() const { return grammar_rules_; }
};

} // namespace ideam::godot_ext
