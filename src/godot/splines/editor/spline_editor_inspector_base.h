#pragma once

/**
 * spline_editor_inspector_base.h
 * Base EditorInspectorPlugin for the IdeamSplines module (`src/godot/splines/editor/`),
 * inheriting shared Undo/Redo injection and property-filtering infrastructure from
 * `GeometryEditorInspectorBase` (`src/godot/geometry/editor/`).
 */

#include "../../geometry/editor/geometry_editor_inspector_base.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class SplineEditorInspectorBase : public GeometryEditorInspectorBase {
    GDCLASS(SplineEditorInspectorBase, GeometryEditorInspectorBase)

protected:
    static void _bind_methods();

    [[nodiscard]] bool _is_managed_property(
        godot::Object *p_object,
        const godot::String &p_name
    ) const override;

public:
    SplineEditorInspectorBase() = default;
    ~SplineEditorInspectorBase() override = default;

    void set_hide_managed_spline_properties(bool p_hide) {
        set_hide_managed_geometry_properties(p_hide);
    }
    [[nodiscard]] bool get_hide_managed_spline_properties() const noexcept {
        return get_hide_managed_geometry_properties();
    }

#ifdef TOOLS_ENABLED
    godot::EditorUndoRedoManager *get_undo_redo() const override;
#endif

    bool _can_handle(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext