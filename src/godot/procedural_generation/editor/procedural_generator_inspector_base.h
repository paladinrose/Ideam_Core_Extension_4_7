#pragma once

/**
 * procedural_generator_inspector_base.h
 * Shared base `EditorInspectorPlugin` for `IdeamProceduralGeneration` and `IdeamWaveFunctionCollapse`
 * (`src/godot/procedural_generation/editor/`).
 *
 * Parallels `GeometryEditorInspectorBase` (which serves `IdeamGeometry`, `IdeamSplines`, and `IdeamTiles`).
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../procedural_generator_resource.h"

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
};

} // namespace ideam::godot_ext
