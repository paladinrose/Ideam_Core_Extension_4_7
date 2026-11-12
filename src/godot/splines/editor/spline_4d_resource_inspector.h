#pragma once

/**
 * spline_4d_resource_inspector.h
 * EditorInspectorPlugin for `Spline4DResource` (`src/godot/splines/editor/`).
 *
 * Responsibilities:
 *   1. Identifies `Spline4DResource` instances in the Godot Inspector (`_can_handle`).
 *   2. Inherits automatic `IdeamUndoRedo` + `EditorUndoRedoManager` injection and
 *      managed-property filtering from `SplineEditorInspectorBase`.
 *   3. Instantiates and mounts `ColorSplineControl` when `color_gradient_mode` is active
 *      on `Spline4DResource` (or provides a live mode-switcher tab/header between
 *      `Spline4DControl` and `ColorSplineControl`).
 */

#include "spline_editor_inspector_base.h"

namespace ideam::godot_ext {

class Spline4DResourceInspector : public SplineEditorInspectorBase {
    GDCLASS(Spline4DResourceInspector, SplineEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    Spline4DResourceInspector();
    ~Spline4DResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext