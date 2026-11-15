#pragma once

/**
 * characters_inspector.h
 * EditorInspectorPlugin for `IdeamCharacters` (`CharacterBlueprintResource`,
 * `CharacterApparelResource`, `ParametricCharacterNode3D`, and `DataOrientedCharacters`)
 * in `src/godot/characters/editor/`.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../character_blueprint_resource.h"
#include "../ui/character_morph_slider_panel.h"
#include "../../tasks/domains/characters/data_oriented_characters.h"

namespace ideam::godot_ext {

class CharactersInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(CharactersInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    CharactersInspector() = default;
    ~CharactersInspector() override = default;

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
