#include "geometry_editor_inspector_base.h"

#include "../geometry_resource.h"
#include "../ideam_geometry_plugin.h"
#include "../ui/geometry_canvas_2d_base.h"
#include "../../splines/ideam_splines_plugin.h"
#include "../../tiles/ideam_tiles_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void GeometryEditorInspectorBase::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_editor_root", "root"), &GeometryEditorInspectorBase::set_editor_root);
    ClassDB::bind_method(D_METHOD("get_editor_root"), &GeometryEditorInspectorBase::get_editor_root);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "editor_root", PROPERTY_HINT_NODE_TYPE, "Control"),
        "set_editor_root",
        "get_editor_root"
    );

    ClassDB::bind_method(
        D_METHOD("set_hide_managed_geometry_properties", "hide"),
        &GeometryEditorInspectorBase::set_hide_managed_geometry_properties
    );
    ClassDB::bind_method(
        D_METHOD("get_hide_managed_geometry_properties"),
        &GeometryEditorInspectorBase::get_hide_managed_geometry_properties
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "hide_managed_geometry_properties"),
        "set_hide_managed_geometry_properties",
        "get_hide_managed_geometry_properties"
    );
}

void GeometryEditorInspectorBase::set_editor_root(godot::Control *p_root) {
    editor_root = p_root;
}

void GeometryEditorInspectorBase::set_hide_managed_geometry_properties(bool p_hide) {
    hide_managed_geometry_properties = p_hide;
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *GeometryEditorInspectorBase::get_undo_redo() const {
    if (godot::EditorUndoRedoManager *geom_ur = IdeamGeometryPlugin::get_static_undo_redo()) {
        return geom_ur;
    }
    if (godot::EditorUndoRedoManager *splines_ur = IdeamSplinesPlugin::get_static_undo_redo()) {
        return splines_ur;
    }
    return IdeamTilesPlugin::get_static_undo_redo();
}
#endif

bool GeometryEditorInspectorBase::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<GeometryResource>(p_object) != nullptr;
}

void GeometryEditorInspectorBase::_parse_begin(godot::Object *p_object) {
    if (!p_object) return;

    // 1. Inject IdeamUndoRedo into `p_object->get("undo_redo")` first.
    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    // 2. Instantiate and mount the subclass-specific custom Inspector UI control.
    if (godot::Control *custom_ctrl = _create_inspector_control(p_object)) {
        add_custom_control(custom_ctrl);
    }
}

bool GeometryEditorInspectorBase::_parse_property(
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

    if (hide_managed_geometry_properties && _is_managed_property(p_object, p_name)) {
        return true;
    }
    return false;
}

// ============================================================================
// Polygon2DResourceInspector Implementation
// ============================================================================

bool Polygon2DResourceInspector::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<Polygon2DResource>(p_object) != nullptr;
}

godot::Control *Polygon2DResourceInspector::_create_inspector_control(godot::Object *p_object) {
    Polygon2DResource *poly = godot::Object::cast_to<Polygon2DResource>(p_object);
    if (!poly) return nullptr;

    Polygon2DCanvas *canvas = memnew(Polygon2DCanvas);
    canvas->set_polygon_resource(godot::Ref<Polygon2DResource>(poly));
    return canvas;
}

} // namespace ideam::godot_ext
