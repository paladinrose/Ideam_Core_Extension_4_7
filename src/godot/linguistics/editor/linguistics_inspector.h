#pragma once

/**
 * linguistics_inspector.h
 * EditorInspectorPlugin for `IdeamLinguistics` resources (`SememeResource`,
 * `SemanticTagSetResource`, `GrammarRuleResource`, `LexiconResource`,
 * `LanguageAssetResource`) in `src/godot/linguistics/editor/`.
 *
 * Responsibilities:
 *   1. Injects `IdeamUndoRedo` via `IdeamEditorInspectorPlugin::_parse_begin()`.
 *   2. Renders an interactive "Componential Bitmask & Prompt Affinity Test Bench"
 *      header when inspecting `SememeResource` or `SemanticTagSetResource`.
 *   3. Renders an interactive "Lexicon Telemetry, Built-In Bootstrap & Morphology/Phoneme
 *      Decomposer" header when inspecting `LexiconResource` or `LanguageAssetResource`.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../lexicon_resource.h"
#include "../semantic_prompt_parser.h"
#include "../sememe_resource.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace ideam::godot_ext {

class LinguisticsInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(LinguisticsInspector, IdeamEditorInspectorPlugin)

private:
    godot::ObjectID current_object_id_;
    godot::ObjectID summary_label_id_;
    godot::ObjectID test_result_label_id_;

    void _refresh_summary_panel();
    void _on_populate_builtin_vocab_pressed();
    void _on_test_input_submitted(const godot::String &p_text);

protected:
    static void _bind_methods();

public:
    LinguisticsInspector() = default;
    ~LinguisticsInspector() override = default;

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(
        godot::Object *p_object,
        godot::Variant::Type p_type,
        const godot::String &p_name,
        godot::PropertyHint p_hint_type,
        const godot::String &p_hint_string,
        godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
        bool p_wide
    ) override;
};

} // namespace ideam::godot_ext
