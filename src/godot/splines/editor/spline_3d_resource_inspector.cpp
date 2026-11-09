#include "spline_3d_resource_inspector.h"

#include "../spline_3d/spline_3d_control.h"
#include "../spline_3d/spline_3d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void Spline3DResourceInspector::_bind_methods() {
}

Spline3DResourceInspector::Spline3DResourceInspector() = default;

Spline3DResourceInspector::~Spline3DResourceInspector() = default;

bool Spline3DResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<Spline3DResource>(p_object) != nullptr;
}

godot::Control *Spline3DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Spline3DResource *spline_3d = godot::Object::cast_to<Spline3DResource>(p_object);
    if (!spline_3d) {
        return nullptr;
    }

    // By the time `_create_inspector_control` is called, `SplineEditorInspectorBase::_parse_begin`
    // has already injected a valid `Ref<IdeamUndoRedo>` (wired to `EditorUndoRedoManager`)
    // into `spline_3d->undo_redo`.
    Spline3DControl *ctrl = memnew(Spline3DControl);
    ctrl->set_spline_3d(godot::Ref<Spline3DResource>(spline_3d));
    return ctrl;
}

} // namespace ideam::godot_ext