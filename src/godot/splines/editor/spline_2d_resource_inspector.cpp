#include "spline_2d_resource_inspector.h"

#include "../spline_2d/spline_2d_control.h"
#include "../spline_2d/spline_2d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void Spline2DResourceInspector::_bind_methods() {
}

Spline2DResourceInspector::Spline2DResourceInspector() = default;

Spline2DResourceInspector::~Spline2DResourceInspector() = default;

bool Spline2DResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<Spline2DResource>(p_object) != nullptr;
}

godot::Control *Spline2DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Spline2DResource *spline_2d = godot::Object::cast_to<Spline2DResource>(p_object);
    if (!spline_2d) {
        return nullptr;
    }

    // By the time `_create_inspector_control` is called, `SplineEditorInspectorBase::_parse_begin`
    // has already injected a valid `Ref<IdeamUndoRedo>` (wired to `EditorUndoRedoManager`)
    // into `spline_2d->undo_redo`.
    Spline2DControl *ctrl = memnew(Spline2DControl);
    ctrl->set_spline_2d(godot::Ref<Spline2DResource>(spline_2d));
    return ctrl;
}

} // namespace ideam::godot_ext