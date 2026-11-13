#pragma once

/**
 * ideam_quantum_darwinism_plugin.h
 * Editor plugin entry point for the `IdeamQuantumDarwinism` (`IdeamQD`) module
 * (`src/godot/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Leans on `IdeamProceduralGeneration` (`src/godot/procedural_generation/`) and
 * works synergistically with `IdeamWaveFunctionCollapse` (`src/godot/wave_function_collapse/`)
 * and `IdeamTasks` (`TaskGraphDOD`).
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/quantum_darwinism_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamQuantumDarwinismPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamQuantumDarwinismPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamQuantumDarwinismPlugin *singleton;

    static constexpr std::string_view QD_SETTINGS_PATH =
        "res://addons/ideam_quantum_darwinism/resources/project_quantum_darwinism_settings.res";

    godot::Ref<QuantumDarwinismInspector> qd_inspector_;

public:
    IdeamQuantumDarwinismPlugin();
    ~IdeamQuantumDarwinismPlugin() override;

    [[nodiscard]] static IdeamQuantumDarwinismPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamQuantumDarwinism");
    }

    // --- Foundation & Lateral Ecosystem Handshakes ---
    [[nodiscard]] bool is_procedural_generation_handshake_available() const {
        return is_plugin_active("IdeamProceduralGeneration");
    }
    [[nodiscard]] bool is_wfc_handshake_available() const {
        return is_plugin_active("IdeamWaveFunctionCollapse") || is_plugin_active("IdeamWFC");
    }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_minds_handshake_available() const { return is_plugin_active("IdeamMinds"); }
    [[nodiscard]] bool is_narratives_handshake_available() const { return is_plugin_active("IdeamNarratives"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

using IdeamQDPlugin = IdeamQuantumDarwinismPlugin;

} // namespace ideam::godot_ext
