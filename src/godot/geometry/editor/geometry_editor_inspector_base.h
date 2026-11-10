#pragma once

/**
 * geometry_editor_inspector_base.h
 * Shared base `EditorInspectorPlugin` for the IdeamGeometry, IdeamSplines, and IdeamTiles modules
 * (`src/godot/geometry/editor/`).
 *
 * Extends `IdeamEditorInspectorPlugin` to centralize:
 *   1. Automatic `EditorUndoRedoManager` resolution via `IdeamGeometryPlugin::get_static_undo_redo()`
 *      (with seamless fallback to `IdeamSplinesPlugin::get_static_undo_redo()`).
 *   2. Guaranteed `IdeamUndoRedo` injection in `_parse_begin()` BEFORE the custom UI control
 *      is instantiated and mounted via `_create_inspector_control()`.
 *   3. Optional suppression of raw serialized array properties (`hide_managed_geometry_properties`)
 *      when those properties are already edited interactively in the mounted custom control.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../resources/polygon_2d_resource.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class GeometryEditorInspectorBase : public IdeamEditorInspectorPlugin {
    GDCLASS(GeometryEditorInspectorBase, IdeamEditorInspectorPlugin)

private:
    godot::Control *editor_root           = nullptr;
    bool hide_managed_geometry_properties = true;

protected:
    static void _bind_methods();

    /**
     * Subclasses override this hook to instantiate and bind their custom Inspector UI control
     * after `IdeamEditorInspectorPlugin::_parse_begin` has injected `IdeamUndoRedo`.
     */
    [[nodiscard]] virtual godot::Control *_create_inspector_control(godot::Object *p_object) {
        (void)p_object;
        return nullptr;
    }

    /**
     * Returns true if `p_name` is managed interactively by the custom control and should
     * have its default Inspector property row omitted when `hide_managed_geometry_properties` is true.
     */
    [[nodiscard]] virtual bool _is_managed_property(
        godot::Object *p_object,
        const godot::String &p_name
    ) const {
        (void)p_object;
        (void)p_name;
        return false;
    }

public:
    GeometryEditorInspectorBase() = default;
    ~GeometryEditorInspectorBase() override = default;

    void set_editor_root(godot::Control *p_root);
    [[nodiscard]] godot::Control *get_editor_root() const noexcept { return editor_root; }

    void set_hide_managed_geometry_properties(bool p_hide);
    [[nodiscard]] bool get_hide_managed_geometry_properties() const noexcept {
        return hide_managed_geometry_properties;
    }

    // --- IdeamEditorInspectorPlugin Overrides ---
#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) override;
    void _parse_begin(godot::Object *p_object) override;
    bool _parse_property(
        godot::Object *p_object,
        godot::Variant::Type p_type,
        const godot::String &p_name,
        godot::PropertyHint p_hint_type,
        const godot::String &p_hint_string,
        godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
        bool p_wide
    ) override;
};

/**
 * Polygon2DResourceInspector
 * Concrete Inspector Plugin mounting a `Polygon2DCanvas` whenever a `Polygon2DResource`
 * is inspected in the Godot Editor.
 */
class Polygon2DResourceInspector : public GeometryEditorInspectorBase {
    GDCLASS(Polygon2DResourceInspector, GeometryEditorInspectorBase)

protected:
    static void _bind_methods() {}

    [[nodiscard]] godot::Control *_create_inspector_control(godot::Object *p_object) override;

public:
    Polygon2DResourceInspector() = default;
    ~Polygon2DResourceInspector() override = default;

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
