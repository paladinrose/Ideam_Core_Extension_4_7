#pragma once

/**
 * ideam_wave_function_collapse_plugin.h
 * Editor plugin entry point for the `IdeamWaveFunctionCollapse` (`IdeamWFC`) module
 * (`src/godot/wave_function_collapse/`).
 *
 * Leans on `IdeamProceduralGeneration` (`src/godot/procedural_generation/`) just as
 * `IdeamSplines` and `IdeamTiles` lean on `IdeamGeometry`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "wfc_rule_tensor_resource.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamWaveFunctionCollapsePlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamWaveFunctionCollapsePlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamWaveFunctionCollapsePlugin *singleton;

    static constexpr std::string_view WFC_SETTINGS_PATH =
        "res://addons/ideam_wave_function_collapse/resources/project_wave_function_collapse_settings.res";

    godot::Ref<WFCRuleTensorInspector> wfc_rule_tensor_inspector_;

public:
    IdeamWaveFunctionCollapsePlugin();
    ~IdeamWaveFunctionCollapsePlugin() override;

    [[nodiscard]] static IdeamWaveFunctionCollapsePlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamWaveFunctionCollapse");
    }

    // --- Foundation & Lateral Ecosystem Handshakes ---
    [[nodiscard]] bool is_procedural_generation_handshake_available() const {
        return is_plugin_active("IdeamProceduralGeneration");
    }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_design_handshake_available() const { return is_plugin_active("IdeamDesign"); }
    [[nodiscard]] bool is_narratives_handshake_available() const { return is_plugin_active("IdeamNarratives"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

using IdeamWFCPlugin = IdeamWaveFunctionCollapsePlugin;

} // namespace ideam::godot_ext
