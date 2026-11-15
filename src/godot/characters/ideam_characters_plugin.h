#pragma once

/**
 * ideam_characters_plugin.h
 * Editor plugin entry point for the `IdeamCharacters` module (`src/godot/characters/`).
 *
 * Responsibilities:
 *   1. Manages the `"IdeamCharacters"` active-plugin handshake in `.ideam_registry.cfg`.
 *   2. Registers `CharactersInspector` (`CharacterMorphSliderPanel` + telemetry) in the Godot Editor.
 *   3. Exposes lateral ecosystem handshake queries (`is_splines_handshake_available()`,
 *      `is_design_handshake_available()`, `is_geometry_handshake_available()`,
 *      `is_linguistics_handshake_available()`, `is_games_handshake_available()`,
 *      `is_tasks_handshake_available()`).
 *   4. Provides static `EditorUndoRedoManager` / `IdeamUndoRedo` helpers for
 *      `CharacterBlueprintResource`, `CharacterApparelResource`, and `ParametricCharacterNode3D`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/characters_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamCharactersPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamCharactersPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamCharactersPlugin *singleton;

    static constexpr std::string_view CHARACTERS_SETTINGS_PATH =
        "res://addons/ideam_characters/resources/project_characters_settings.res";

    godot::Ref<CharactersInspector> inspector_plugin_;

public:
    IdeamCharactersPlugin();
    ~IdeamCharactersPlugin() override;

    [[nodiscard]] static IdeamCharactersPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamCharacters"); }

    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_design_handshake_available() const { return is_plugin_active("IdeamDesign"); }
    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_linguistics_handshake_available() const { return is_plugin_active("IdeamLinguistics"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
