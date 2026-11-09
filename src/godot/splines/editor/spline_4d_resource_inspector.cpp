#include "spline_4d_resource_inspector.h"

#include "../spline_4d/spline_4d_control.h"
#include "../spline_4d/spline_4d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void Spline4DResourceInspector::_bind_methods() {
}

Spline4DResourceInspector::Spline4DResourceInspector() = default;

Spline4DResourceInspector::~Spline4DResourceInspector() = default;

bool Spline4DResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<Spline4DResource>(p_object) != nullptr;
}

godot::Control *Spline4DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Spline4DResource *spline_4d = godot::Object::cast_to<Spline4DResource>(p_object);
    if (!spline_4d) {
        return nullptr;
    }

    // By the time `_create_inspector_control` is called, `SplineEditorInspectorBase::_parse_begin`
    // has already injected a valid `Ref<IdeamUndoRedo>` (wired to `EditorUndoRedoManager`)
    // into `spline_4d->undo_redo`.
    Spline4DControl *ctrl = memnew(Spline4DControl);
    ctrl->set_spline_4d(godot::Ref<Spline4DResource>(spline_4d));
    return ctrl;
}

} // namespace ideam::godot_ext