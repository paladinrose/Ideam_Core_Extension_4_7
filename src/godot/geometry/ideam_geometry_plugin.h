#pragma once

/**
 * ideam_geometry_plugin.h
 * Editor plugin entry point for the shared IdeamGeometry module (`src/godot/geometry/`).
 *
 * Responsibilities:
 *   - Manages the `"IdeamGeometry"` active-plugin handshake in `res://.ideam_registry.cfg`.
 *   - Registers `GeometryTaskDomainPack` with `IdeamTaskRegistry` when `IdeamTasks` is active.
 *   - Provides static `EditorUndoRedoManager` and `IdeamUndoRedo` helpers shared across
 *     `GeometryResource`, `Polygon2DResource`, `VoronoiDiagram2DResource`, `SDFGrid2DResource`,
 *     `GeometryCanvas2DBase`, and downstream geometry plugins (`IdeamSplines`, `IdeamTiles`).
 *   - Registers `Polygon2DResourceInspector`, `VoronoiDiagram2DResourceInspector`, and
 *     `SDFGrid2DResourceInspector` during `_enter_tree()` and cleans them up in `_exit_tree()`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/geometry_editor_inspector_base.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamGeometryPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamGeometryPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamGeometryPlugin *singleton;

    static constexpr std::string_view GEOMETRY_SETTINGS_PATH =
        "res://addons/ideam_geometry/resources/project_geometry_settings.res";

    godot::Ref<Polygon2DResourceInspector> polygon_2d_inspector;
    godot::Ref<VoronoiDiagram2DResourceInspector> voronoi_2d_inspector;
    godot::Ref<SDFGrid2DResourceInspector> sdf_grid_2d_inspector;

public:
    IdeamGeometryPlugin();
    ~IdeamGeometryPlugin() override;

    [[nodiscard]] static IdeamGeometryPlugin *get_singleton() noexcept { return singleton; }

    // --- EditorPlugin Lifecycle ---
    void _enter_tree() override;
    void _exit_tree() override;

    // --- Cross-Plugin Ecosystem & TaskDomainPack Handshake ---
    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamGeometry");
    }
    void register_task_domain_pack();

    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }
    [[nodiscard]] bool is_cymatics_handshake_available() const { return is_plugin_active("IdeamCymatics"); }

    // --- Static Undo/Redo Bridge for Geometry Resources, Canvases, Inspectors & Gizmos ---
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext