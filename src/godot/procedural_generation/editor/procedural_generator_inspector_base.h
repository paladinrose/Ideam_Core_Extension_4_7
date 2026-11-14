#pragma once

/**
 * procedural_generator_inspector_base.h
 * Shared base `EditorInspectorPlugin` for `IdeamProceduralGeneration` and `IdeamWaveFunctionCollapse`
 * (`src/godot/procedural_generation/editor/`).
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../procedural_patterner.h"

namespace ideam::godot_ext {

class ProceduralGeneratorInspectorBase : public IdeamEditorInspectorPlugin {
    GDCLASS(ProceduralGeneratorInspectorBase, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    ProceduralGeneratorInspectorBase() = default;
    ~ProceduralGeneratorInspectorBase() override = default;

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
