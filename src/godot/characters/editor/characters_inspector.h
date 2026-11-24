#pragma once

/**
 * characters_inspector.h
 * EditorInspectorPlugin for `IdeamCharacters` (`BodyPlanResource`, `LoomisCharacter3DResource`,
 * `LoomisCharacterNode3D`, `LoomisCharacter3DResource`, `CharacterApparelResource`,
 * `LoomisCharacterNode3D`, and `DataOrientedCharacters`) -> opens Tier 2 `CharacterModeler`
 * and dispatches `IdeamTutorials` help topics.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../body_plan_resource.h"
#include "../character_apparel_resource.h"
#include "../loomis_character_3d_resource.h"
#include "../loomis_character_node_3d.h"
#include "../ui/character_morph_slider_panel.h"
#include "../../tasks/domains/characters/data_oriented_characters.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class CharactersInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(CharactersInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    CharactersInspector() = default;
    ~CharactersInspector() override = default;

    void _on_open_in_character_modeler(const godot::Ref<godot::Resource> &p_resource);
    void _on_help_pressed(const godot::StringName &p_topic_id);

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(godot::Object *p_object, godot::Variant::Type p_type, const godot::String &p_name, godot::PropertyHint p_hint_type, const godot::String &p_hint_string, godot::BitField<godot::PropertyUsageFlags> p_usage_flags, bool p_wide) override { return false; }
};

} // namespace ideam::godot_ext
