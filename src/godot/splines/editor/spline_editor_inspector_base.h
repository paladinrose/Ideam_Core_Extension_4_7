#pragma once

/**
 * spline_editor_inspector_base.h
 * Base EditorInspectorPlugin for the IdeamSplines module (`src/godot/splines/editor/`).
 *
 * Extends `IdeamEditorInspectorPlugin` to centralize three responsibilities shared across
 * all dimensional spline inspectors (`Spline1DResourceInspector` .. `Spline4DResourceInspector`):
 *   1. Automatic `EditorUndoRedoManager` resolution via `IdeamSplinesPlugin::get_static_undo_redo()`.
 *   2. Guaranteed `IdeamUndoRedo` injection in `_parse_begin()` BEFORE the custom UI control
 *      (`Spline1DControl`, `Spline2DControl`, etc.) is instantiated and bound via
 *      `_create_inspector_control()`.
 *   3. Automatic suppression of redundant raw serialization array editors (`points`,
 *      `back_handles`, `fore_handles`, `handle_flags`, `section_types`, `section_weights`,
 *      `wrap`, `default_curve_type`, `default_weight`) in `_parse_property()`, since those
 *      are already edited interactively inside the mounted `SplineControlBase` subclass.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

class SplineEditorInspectorBase : public IdeamEditorInspectorPlugin {
    GDCLASS(SplineEditorInspectorBase, IdeamEditorInspectorPlugin)

private:
    godot::Control *editor_root         = nullptr;
    bool hide_managed_spline_properties = true;

protected:
    static void _bind_methods();

    /**
     * Subclasses override this hook to instantiate and configure their dedicated UI control
     * (e.g., `Spline1DControl`) after `IdeamEditorInspectorPlugin::_parse_begin` has already
     * injected `IdeamUndoRedo` into `p_object`.
     */
    [[nodiscard]] virtual godot::Control *_create_inspector_control(godot::Object *p_object) {
        return nullptr;
    }

    /**
     * Returns true if `p_name` is already managed by the custom spline UI control and should
     * have its default Inspector property row hidden when `hide_managed_spline_properties` is true.
     */
    [[nodiscard]] virtual bool _is_managed_property(
        godot::Object *p_object,
        const godot::String &p_name
    ) const;

public:
    SplineEditorInspectorBase();
    ~SplineEditorInspectorBase() override;

    // --- Optional Editor Root Reference (Matches GameAgentActionEditorInspectorPlugin) ---
    void set_editor_root(godot::Control *p_root);
    [[nodiscard]] godot::Control *get_editor_root() const noexcept { return editor_root; }

    // --- Raw Serialization Property Visibility Toggle ---
    void set_hide_managed_spline_properties(bool p_hide);
    [[nodiscard]] bool get_hide_managed_spline_properties() const noexcept {
        return hide_managed_spline_properties;
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

} // namespace ideam::godot_ext