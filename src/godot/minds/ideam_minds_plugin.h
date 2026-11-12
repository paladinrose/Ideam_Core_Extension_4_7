#pragma once

/**
 * ideam_minds_plugin.h
 * Editor plugin entry point for the `IdeamMinds` module (`src/godot/minds/`).
 *
 * Responsibilities:
 *   1. Manages the `"IdeamMinds"` active-plugin handshake in `.ideam_registry.cfg`.
 *   2. Exposes lateral ecosystem handshake queries (`is_narratives_handshake_available()`,
 *      `is_games_handshake_available()`, `is_geometry_handshake_available()`,
 *      `is_tiles_handshake_available()`, `is_tasks_handshake_available()`).
 *   3. Provides static `EditorUndoRedoManager` / `IdeamUndoRedo` helpers for
 *      `InformationMatrixResource` and `AgentMindResource`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamMindsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamMindsPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamMindsPlugin *singleton;

    static constexpr std::string_view MINDS_SETTINGS_PATH =
        "res://addons/ideam_minds/resources/project_minds_settings.res";

public:
    IdeamMindsPlugin();
    ~IdeamMindsPlugin() override;

    [[nodiscard]] static IdeamMindsPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamMinds"); }

    [[nodiscard]] bool is_narratives_handshake_available() const { return is_plugin_active("IdeamNarratives"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
