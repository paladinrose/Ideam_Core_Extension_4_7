#include "spline_editor_inspector_base.h"

#include "../ideam_splines_plugin.h"
#include "../spline_resource.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void SplineEditorInspectorBase::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_editor_root", "root"), &SplineEditorInspectorBase::set_editor_root);
    ClassDB::bind_method(D_METHOD("get_editor_root"), &SplineEditorInspectorBase::get_editor_root);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "editor_root", PROPERTY_HINT_NODE_TYPE, "Control"),
        "set_editor_root",
        "get_editor_root"
    );

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

SplineEditorInspectorBase::SplineEditorInspectorBase() = default;

SplineEditorInspectorBase::~SplineEditorInspectorBase() = default;

void SplineEditorInspectorBase::set_editor_root(godot::Control *p_root) {
    editor_root = p_root;
}

void SplineEditorInspectorBase::set_hide_managed_spline_properties(bool p_hide) {
    hide_managed_spline_properties = p_hide;
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *SplineEditorInspectorBase::get_undo_redo() const {
    return IdeamSplinesPlugin::get_static_undo_redo();
}
#endif

bool SplineEditorInspectorBase::_can_handle(godot::Object *p_object) {
    if (!p_object) {
        return false;
    }
    return godot::Object::cast_to<SplineResource>(p_object) != nullptr;
}

void SplineEditorInspectorBase::_parse_begin(godot::Object *p_object) {
    if (!p_object) {
        return;
    }

    // 1. Always run IdeamEditorInspectorPlugin::_parse_begin first so `p_object->get("undo_redo")`
    //    is detected and populated with a fresh `IdeamUndoRedo` bound to `get_undo_redo()`.
    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    // 2. Ask the concrete inspector subclass to build and bind its custom UI control.
    if (godot::Control *custom_ctrl = _create_inspector_control(p_object)) {
        add_custom_control(custom_ctrl);
    }
}

bool SplineEditorInspectorBase::_is_managed_property(
    godot::Object *p_object,
    const godot::String &p_name
) const {
    (void)p_object;
    // Properties already exposed interactively by SplineControlBase and its 1D..4D subclasses:
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

bool SplineEditorInspectorBase::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    (void)p_type;
    (void)p_hint_type;
    (void)p_hint_string;
    (void)p_usage_flags;
    (void)p_wide;

    if (hide_managed_spline_properties && _is_managed_property(p_object, p_name)) {
        // Returning true instructs the Godot Inspector to omit the default property editor
        // for this property, since our custom control handles it.
        return true;
    }
    return false;
}

} // namespace ideam::godot_ext