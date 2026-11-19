#pragma once

/**
 * characters_inspector.h
 * Thin EditorInspectorPlugin launcher for `IdeamCharacters` (`CharacterBlueprintResource`,
 * `CharacterApparelResource`, `ParametricCharacterNode3D`, and `DataOrientedCharacters`)
 * -> opens the full workspace in Tier 2 `CharacterModeler`.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../character_blueprint_resource.h"
#include "../ui/character_morph_slider_panel.h"
#include "../../tasks/domains/characters/data_oriented_characters.h"

#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

class CharactersInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(CharactersInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    CharactersInspector() = default;
    ~CharactersInspector() override = default;

    void _on_open_in_character_modeler(const godot::Ref<godot::Resource> &p_resource);

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
