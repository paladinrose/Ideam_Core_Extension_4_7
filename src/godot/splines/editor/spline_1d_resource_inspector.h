#pragma once

/**
 * spline_1d_resource_inspector.h
 * EditorInspectorPlugin for `Spline1DResource` (`src/godot/splines/editor/`).
 *
 * Responsibilities:
 *   1. Identifies `Spline1DResource` instances in the Godot Inspector (`_can_handle`).
 *   2. Inherits automatic `IdeamUndoRedo` + `EditorUndoRedoManager` injection from
 *      `SplineEditorInspectorBase::_parse_begin()`.
 *   3. Instantiates and mounts `Spline1DControl` (including `Spline1DEqualizerCanvas`
 *      and the alternating Point/Section chain) as the resource's primary Inspector UI.
 */

#include "spline_editor_inspector_base.h"

namespace ideam::godot_ext {

class Spline1DResourceInspector : public SplineEditorInspectorBase {
    GDCLASS(Spline1DResourceInspector, SplineEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    Spline1DResourceInspector();
    ~Spline1DResourceInspector() override;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext