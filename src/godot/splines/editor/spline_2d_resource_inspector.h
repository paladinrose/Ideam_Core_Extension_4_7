#pragma once

/**
 * spline_2d_resource_inspector.h
 * EditorInspectorPlugin for `Spline2DResource` (`src/godot/splines/editor/`).
 *
 * Responsibilities:
 *   1. Identifies `Spline2DResource` instances in the Godot Inspector (`_can_handle`).
 *   2. Inherits automatic `IdeamUndoRedo` + `EditorUndoRedoManager` injection and
 *      managed-property filtering from `SplineEditorInspectorBase`.
 *   3. Instantiates and mounts `Spline2DControl` (including `Spline2DCanvas`
 *      and the alternating 2D Point/Section chain) as the resource's primary Inspector UI.
 */

#include "spline_editor_inspector_base.h"

namespace ideam::godot_ext {

class Spline2DResourceInspector : public SplineEditorInspectorBase {
    GDCLASS(Spline2DResourceInspector, SplineEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    Spline2DResourceInspector();
    ~Spline2DResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext