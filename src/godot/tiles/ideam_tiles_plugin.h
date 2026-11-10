#pragma once

/**
 * ideam_tiles_plugin.h
 * Editor plugin entry point for the `IdeamTiles` module (`src/godot/tiles/`).
 *
 * Responsibilities:
 *   1. Manages the `"IdeamTiles"` active-plugin handshake in `.ideam_registry.cfg`.
 *   2. Registers `TilesTaskDomainPack` with `IdeamTaskRegistry` when `IdeamTasks` is active.
 *   3. Registers `WallpaperTilingResourceInspector` and `CompositeTilingResourceInspector`
 *      during `_enter_tree()` and cleans them up in `_exit_tree()`.
 *   4. Exposes lateral ecosystem handshake queries (`is_geometry_handshake_available()`,
 *      `is_splines_handshake_available()`, `is_tasks_handshake_available()`,
 *      `is_games_handshake_available()`) and static `EditorUndoRedoManager` / `IdeamUndoRedo`
 *      helpers for all tiling resources, UI controls, and inspectors.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/composite_tiling_resource_inspector.h"
#include "editor/wallpaper_tiling_resource_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamTilesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTilesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamTilesPlugin *singleton;

    static constexpr std::string_view TILES_SETTINGS_PATH =
        "res://addons/ideam_tiles/resources/project_tiles_settings.res";

    godot::Ref<WallpaperTilingResourceInspector> wallpaper_tiling_inspector_;
    godot::Ref<CompositeTilingResourceInspector> composite_tiling_inspector_;

public:
    IdeamTilesPlugin();
    ~IdeamTilesPlugin() override;

    [[nodiscard]] static IdeamTilesPlugin *get_singleton() noexcept { return singleton; }

    // --- EditorPlugin Lifecycle ---
    void _enter_tree() override;
    void _exit_tree() override;

    // --- Cross-Plugin Ecosystem & TaskDomainPack Handshakes ---
    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamTiles"); }
    void register_task_domain_pack();

    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }

    // --- Static Undo/Redo Bridge for Tiling Resources, Controls & Inspectors ---
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
