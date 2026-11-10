#include "spline_editor_inspector_base.h"

#include "../ideam_splines_plugin.h"
#include "../spline_resource.h"
#include "../../geometry/ideam_geometry_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void SplineEditorInspectorBase::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(
        D_METHOD("set_hide_managed_spline_properties", "hide"),
        &SplineEditorInspectorBase::set_hide_managed_spline_properties
    );
    ClassDB::bind_method(
        D_METHOD("get_hide_managed_spline_properties"),
        &SplineEditorInspectorBase::get_hide_managed_spline_properties
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "hide_managed_spline_properties"),
        "set_hide_managed_spline_properties",
        "get_hide_managed_spline_properties"
    );
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *SplineEditorInspectorBase::get_undo_redo() const {
    if (godot::EditorUndoRedoManager *sp_ur = IdeamSplinesPlugin::get_static_undo_redo()) {
        return sp_ur;
    }
    return IdeamGeometryPlugin::get_static_undo_redo();
}
#endif

bool SplineEditorInspectorBase::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<SplineResource>(p_object) != nullptr;
}

bool SplineEditorInspectorBase::_is_managed_property(
    godot::Object *p_object,
    const godot::String &p_name
) const {
    (void)p_object;
    return p_name == "wrap" ||
           p_name == "default_curve_type" ||
           p_name == "default_weight" ||
           p_name == "points" ||
           p_name == "back_handles" ||
           p_name == "fore_handles" ||
           p_name == "handle_flags" ||
           p_name == "section_types" ||
           p_name == "section_weights";
}

} // namespace ideam::godot_ext