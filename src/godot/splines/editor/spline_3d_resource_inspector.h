#pragma once

/**
 * spline_3d_resource_inspector.h
 * EditorInspectorPlugin for `Spline3DResource` (`src/godot/splines/editor/`).
 *
 * Responsibilities:
 *   1. Identifies `Spline3DResource` instances in the Godot Inspector (`_can_handle`).
 *   2. Inherits automatic `IdeamUndoRedo` + `EditorUndoRedoManager` injection and
 *      managed-property filtering from `SplineEditorInspectorBase`.
 *   3. Instantiates and mounts `Spline3DControl` (with 3D presets, live AABB telemetry,
 *      and the alternating 3D Point/Section chain) as the resource's primary Inspector UI.
 */

#include "spline_editor_inspector_base.h"

namespace ideam::godot_ext {

class Spline3DResourceInspector : public SplineEditorInspectorBase {
    GDCLASS(Spline3DResourceInspector, SplineEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    Spline3DResourceInspector();
    ~Spline3DResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext