#pragma once

/**
 * ideam_linguistics_plugin.h
 * Editor plugin entry point for `IdeamLinguistics` (`src/godot/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Provides the universal structural semantics (`Seme` -> `Sememe`), lexicology
 * (`Morpheme`, `Lexeme`, `Lect`), phonology (`PhonemePOD` 16-bit distinctive features),
 * natural-language Wizard prompt parsing (`SemanticPromptParser`), and neuro-symbolic
 * SLM grammar/vocabulary constraints across the Ideam ecosystem.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/linguistics_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamLinguisticsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamLinguisticsPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamLinguisticsPlugin *singleton;

    static constexpr std::string_view LINGUISTICS_SETTINGS_PATH =
        "res://addons/ideam_linguistics/resources/project_linguistics_settings.res";

    godot::Ref<LinguisticsInspector> linguistics_inspector_;

public:
    IdeamLinguisticsPlugin();
    ~IdeamLinguisticsPlugin() override;

    [[nodiscard]] static IdeamLinguisticsPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamLinguistics");
    }

    [[nodiscard]] bool is_communications_handshake_available() const { return is_plugin_active("IdeamCommunications"); }
    [[nodiscard]] bool is_minds_handshake_available() const { return is_plugin_active("IdeamMinds"); }
    [[nodiscard]] bool is_narratives_handshake_available() const { return is_plugin_active("IdeamNarratives"); }
    [[nodiscard]] bool is_procedural_generation_handshake_available() const { return is_plugin_active("IdeamProceduralGeneration"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
