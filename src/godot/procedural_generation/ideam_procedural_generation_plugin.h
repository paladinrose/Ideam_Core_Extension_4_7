#pragma once

/**
 * ideam_procedural_generation_plugin.h
 * Editor plugin entry point for the `IdeamProceduralGeneration` module
 * (`src/godot/procedural_generation/`).
 *
 * Serves as the foundational procedural generation plugin that `IdeamWaveFunctionCollapse`
 * leans on (paralleling how `IdeamSplines` and `IdeamTiles` lean on `IdeamGeometry`).
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

class IdeamProceduralGenerationPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamProceduralGenerationPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamProceduralGenerationPlugin *singleton;

    static constexpr std::string_view PROCGEN_SETTINGS_PATH =
        "res://addons/ideam_procedural_generation/resources/project_procedural_generation_settings.res";

public:
    IdeamProceduralGenerationPlugin();
    ~IdeamProceduralGenerationPlugin() override;

    [[nodiscard]] static IdeamProceduralGenerationPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamProceduralGeneration");
    }

    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_design_handshake_available() const { return is_plugin_active("IdeamDesign"); }
    [[nodiscard]] bool is_wfc_handshake_available() const {
        return is_plugin_active("IdeamWaveFunctionCollapse") || is_plugin_active("IdeamWFC");
    }
    [[nodiscard]] bool is_quantum_darwinism_handshake_available() const {
        return is_plugin_active("IdeamQuantumDarwinism") || is_plugin_active("IdeamQD");
    }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
