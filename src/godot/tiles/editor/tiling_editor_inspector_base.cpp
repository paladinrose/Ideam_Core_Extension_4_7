#include "tiling_editor_inspector_base.h"

#include "../ideam_tiles_plugin.h"
#include "../tiling_resource.h"
#include "../../geometry/ideam_geometry_plugin.h"
#include "../../splines/ideam_splines_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

void TilingEditorInspectorBase::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(
        D_METHOD("set_hide_managed_tiling_properties", "hide"),
        &TilingEditorInspectorBase::set_hide_managed_tiling_properties
    );
    ClassDB::bind_method(
        D_METHOD("get_hide_managed_tiling_properties"),
        &TilingEditorInspectorBase::get_hide_managed_tiling_properties
    );
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "hide_managed_tiling_properties"),
        "set_hide_managed_tiling_properties",
        "get_hide_managed_tiling_properties"
    );
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *TilingEditorInspectorBase::get_undo_redo() const {
    if (godot::EditorUndoRedoManager *tiles_ur = IdeamTilesPlugin::get_static_undo_redo()) {
        return tiles_ur;
    }
    if (godot::EditorUndoRedoManager *geom_ur = IdeamGeometryPlugin::get_static_undo_redo()) {
        return geom_ur;
    }
    return IdeamSplinesPlugin::get_static_undo_redo();
}
#endif

bool TilingEditorInspectorBase::_can_handle(godot::Object *p_object) {
    return p_object != nullptr && godot::Object::cast_to<TilingResource>(p_object) != nullptr;
}

bool TilingEditorInspectorBase::_is_managed_property(
    godot::Object *p_object,
    const godot::String &p_name
) const {
    (void)p_object;
    // Properties interactively managed inside `WallpaperTilingControl` / `CompositeTilingControl`
    return p_name == "authoring_mode" ||
           p_name == "wallpaper_group" ||
           p_name == "tactile_ih_type" ||
           p_name == "shape_preset" ||
           p_name == "tactile_parameters" ||
           p_name == "lattice_scale" ||
           p_name == "lattice_rotation_radians" ||
           p_name == "lattice_origin" ||
           p_name == "coverage_mode" ||
           p_name == "stitch_mode" ||
           p_name == "max_bridge_span" ||
           p_name == "auto_hexize" ||
           p_name == "edge_subdivisions" ||
           p_name == "auto_rebuild";
}

} // namespace ideam::godot_ext
