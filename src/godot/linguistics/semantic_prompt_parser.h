#pragma once

/**
 * semantic_prompt_parser.h
 * `SemanticPromptParser` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. NATURAL-LANGUAGE WIZARD & SMART-TOOL PROMPT PARSING:
 *    - Parses natural language descriptions (such as `"top down, medieval, fantasy RPG"` or
 *      `"first-person VR mystery adventure without combat"`) into a `SemanticTagSetResource`
 *      (192-bit Nuclear + 192-bit Peripheral `Seme` bitmasks + MRL-128 binary embedding +
 *      excluded semes) that can immediately configure:
 *        * `IdeamGames`: `GameplayMode`, `GameplayPerspective`, `GameplayControlMap`, `GameplayStyle`
 *        * `IdeamNarratives`: `Plot` and `Role` archetypes
 *        * `IdeamProceduralGeneration` & `IdeamWaveFunctionCollapse`: `ProceduralPattern` selection
 *        * `IdeamDesign` & `IdeamCharacters`: `Style3DResource` and `LoomisCharacter3DResource` cues
 *
 * 2. FRAMENET & UNIVERSAL DEPENDENCIES (`UPOS` / `DepRel`) CLAUSE PARSING:
 *    - `parse_prompt_to_semantic_frame()` extracts `Agent` (`DEP_NSUBJ`), `Predicate` (`DEP_ROOT`),
 *      `Patient` (`DEP_OBJ`), and `Instrument`/`Location` (`DEP_OBL` via prepositions like `with`,
 *      `in`, `at`, `to`), matches a FrameNet `SemanticFrameResource`, and evaluates thematic role
 *      compatibility (`evaluate_role_fit`).
 *
 * 3. COMPOUND IDENTIFIER & REFLECTION TOKENIZATION:
 *    - Provides `split_compound_identifier()` and `parse_identifier_to_tag_set()` to decompose
 *      Godot C++/GDScript identifiers (`PascalCase`, `camelCase`, `snake_case`, `kebab-case`,
 *      dimensional tags like `2D`/`3D`/`4D`, and acronyms like `XR`/`VR`/`RPG`/`WFC`) into
 *      head-nouns, modifiers, and action verbs for `LinguisticObjectProfiler` and `ProceduralPatterner`.
 */

#include "lexicon_resource.h"
#include "sememe_resource.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class SemanticPromptParser : public godot::RefCounted {
    GDCLASS(SemanticPromptParser, godot::RefCounted)

private:
    mutable godot::Ref<LanguageAssetResource> language_asset_;
    bool treat_modifiers_as_peripheral_ = false;

    void _ensure_default_language_asset() const;

protected:
    static void _bind_methods();

public:
    SemanticPromptParser() = default;
    ~SemanticPromptParser() override = default;

    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) { language_asset_ = p_asset; }
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const;

    void set_treat_modifiers_as_peripheral(bool p_peripheral) { treat_modifiers_as_peripheral_ = p_peripheral; }
    [[nodiscard]] bool get_treat_modifiers_as_peripheral() const noexcept { return treat_modifiers_as_peripheral_; }

    // --- Identifier & Natural-Language Tokenization ---
    [[nodiscard]] static godot::PackedStringArray split_compound_identifier(
        const godot::String &p_identifier
    );

    [[nodiscard]] godot::PackedStringArray tokenize_and_normalize_prompt(
        const godot::String &p_prompt
    ) const;

    [[nodiscard]] godot::Array extract_structured_tokens(
        const godot::String &p_prompt
    ) const;

    // --- Prompt & Identifier -> SemanticTagSetResource Compilation ---
    [[nodiscard]] godot::Ref<SemanticTagSetResource> parse_prompt_to_tag_set(
        const godot::String &p_prompt
    ) const;

    [[nodiscard]] godot::Ref<SemanticTagSetResource> parse_identifier_to_tag_set(
        const godot::String &p_identifier,
        bool p_head_noun_only_nuclear = true
    ) const;

    // --- FrameNet & Universal Dependencies (`UPOS` / `DepRel`) Clause Extraction ---
    [[nodiscard]] godot::Dictionary parse_prompt_to_semantic_frame(
        const godot::String &p_prompt
    ) const;

    // --- Prompt Similarity & SLM / GBNF Constraint Helpers ---
    [[nodiscard]] float score_prompt_similarity(
        const godot::String &p_prompt_a,
        const godot::String &p_prompt_b
    ) const;

    [[nodiscard]] godot::Dictionary compile_prompt_to_slm_constraints(
        const godot::String &p_prompt,
        int p_lect_id = 0
    ) const;
};

} // namespace ideam::godot_ext
