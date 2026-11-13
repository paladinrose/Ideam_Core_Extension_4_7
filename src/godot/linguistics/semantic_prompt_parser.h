#pragma once

/**
 * semantic_prompt_parser.h
 * `SemanticPromptParser` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Parses natural language descriptions (such as `"top down, medieval, fantasy RPG"`)
 * into a `SemanticTagSetResource` (Nuclear + Peripheral `Seme` bitmasks) that can
 * immediately configure:
 *   - `IdeamGames`: `GameplayMode`, `GameplayPerspective`, `GameplayControlMap`, `GameplayStyle`
 *   - `IdeamNarratives`: `Plot` and `Role` archetypes
 *   - `IdeamProceduralGeneration` & `IdeamWaveFunctionCollapse`: `ProceduralPattern` selection
 *   - `IdeamDesign` & `IdeamCharacters`: `Style3DResource` and `CharacterBlueprintResource` cues
 */

#include "lexicon_resource.h"
#include "sememe_resource.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class SemanticPromptParser : public godot::RefCounted {
    GDCLASS(SemanticPromptParser, godot::RefCounted)

private:
    godot::Ref<LanguageAssetResource> language_asset_;

protected:
    static void _bind_methods();

public:
    SemanticPromptParser() = default;
    ~SemanticPromptParser() override = default;

    void set_language_asset(const godot::Ref<LanguageAssetResource> &p_asset) { language_asset_ = p_asset; }
    [[nodiscard]] godot::Ref<LanguageAssetResource> get_language_asset() const { return language_asset_; }

    [[nodiscard]] godot::Ref<SemanticTagSetResource> parse_prompt_to_tag_set(
        const godot::String &p_prompt
    ) const;
};

} // namespace ideam::godot_ext
