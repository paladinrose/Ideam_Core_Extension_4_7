#pragma once

/**
 * ideam_design_plugin.h
 * Editor plugin entry point for the `IdeamDesign` module (`src/godot/design/`).
 *
 * Responsibilities:
 *   1. Manages the `"IdeamDesign"` active-plugin handshake in `.ideam_registry.cfg`.
 *   2. Exposes lateral ecosystem handshake queries (`is_geometry_handshake_available()`,
 *      `is_splines_handshake_available()`, `is_tiles_handshake_available()`,
 *      `is_characters_handshake_available()`, `is_wfc_handshake_available()`).
 *   3. Provides static `EditorUndoRedoManager` / `IdeamUndoRedo` helpers for `Style3DResource`,
 *      `Theme3DResource`, `Container3D`, `Socket3D`, and `SemanticStructure3D`.
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

class IdeamDesignPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamDesignPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamDesignPlugin *singleton;

    static constexpr std::string_view DESIGN_SETTINGS_PATH =
        "res://addons/ideam_design/resources/project_design_settings.res";

public:
    IdeamDesignPlugin();
    ~IdeamDesignPlugin() override;

    [[nodiscard]] static IdeamDesignPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamDesign"); }

    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_characters_handshake_available() const { return is_plugin_active("IdeamCharacters"); }
    [[nodiscard]] bool is_wfc_handshake_available() const {
        return is_plugin_active("IdeamWaveFunctionCollapse") || is_plugin_active("IdeamWFC");
    }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
