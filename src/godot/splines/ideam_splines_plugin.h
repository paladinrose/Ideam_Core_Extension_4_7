#pragma once

/**
 * ideam_splines_plugin.h
 * Editor plugin entry point for the IdeamSplines module.
 *
 * Responsibilities:
 *   - Manages the "IdeamSplines" active-plugin handshake in `.ideam_registry.cfg`.
 *   - Exposes static EditorUndoRedoManager and IdeamUndoRedo helpers for SplineResource,
 *     Controls, Inspectors, and 3D Gizmos.
 *   - Registers all four dimensional spline resource inspectors (`Spline1DResourceInspector`,
 *     `Spline2DResourceInspector`, `Spline3DResourceInspector`, `Spline4DResourceInspector`)
 *     during `_enter_tree()` and cleans them up in `_exit_tree()`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/spline_1d_resource_inspector.h"
#include "editor/spline_2d_resource_inspector.h"
#include "editor/spline_3d_resource_inspector.h"
#include "editor/spline_4d_resource_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamSplinesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamSplinesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamSplinesPlugin *singleton;

    static constexpr std::string_view SPLINES_SETTINGS_PATH =
        "res://addons/ideam_splines/resources/project_splines_settings.res";

    // Active Editor Inspector Plugins (1D - 4D)
    godot::Ref<Spline1DResourceInspector> spline_1d_inspector;
    godot::Ref<Spline2DResourceInspector> spline_2d_inspector;
    godot::Ref<Spline3DResourceInspector> spline_3d_inspector;
    godot::Ref<Spline4DResourceInspector> spline_4d_inspector;

    // [FUTURE PHASE] 3D & 4D Viewport Gizmo Plugin References:
    // godot::Ref<Spline3DGizmo> spline_3d_gizmo_plugin;
    // godot::Ref<Spline4DGizmo> spline_4d_gizmo_plugin;

public:
    IdeamSplinesPlugin();
    ~IdeamSplinesPlugin() override;

    [[nodiscard]] static IdeamSplinesPlugin *get_singleton() noexcept { return singleton; }

    // --- EditorPlugin Lifecycle ---
    void _enter_tree() override;
    void _exit_tree() override;

    // --- Cross-Plugin Ecosystem Queries ---
    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamSplines"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

    // --- Static Undo/Redo Bridge for Spline Resources, Controls, Inspectors & Gizmos ---
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext