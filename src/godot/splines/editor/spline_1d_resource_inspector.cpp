#include "spline_1d_resource_inspector.h"

#include "../spline_1d/spline_1d_control.h"
#include "../spline_1d/spline_1d_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace ideam::godot_ext {

void Spline1DResourceInspector::_bind_methods() {
}

Spline1DResourceInspector::Spline1DResourceInspector() = default;

Spline1DResourceInspector::~Spline1DResourceInspector() = default;

bool Spline1DResourceInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<Spline1DResource>(p_object) != nullptr;
}

godot::Control *Spline1DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Spline1DResource *spline_1d = godot::Object::cast_to<Spline1DResource>(p_object);
    if (!spline_1d) {
        return nullptr;
    }

    // By the time `_create_inspector_control` is called, `SplineEditorInspectorBase::_parse_begin`
    // has already injected a valid `Ref<IdeamUndoRedo>` (wired to `EditorUndoRedoManager`)
    // into `spline_1d->undo_redo`.
    Spline1DControl *ctrl = memnew(Spline1DControl);
    ctrl->set_spline_1d(godot::Ref<Spline1DResource>(spline_1d));
    return ctrl;
}

} // namespace ideam::godot_ext