#pragma once

/**
 * ideam_communications_plugin.h
 * Editor plugin entry point for `IdeamCommunications` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Implements the programmatic Shannon-Weaver communication pipeline (`Transmitter`,
 * `Medium`, `Noise`, `Receiver`, `Encoder`, `Decoder`) and Sacks-Schegloff-Jefferson
 * `Discourse` turn-taking, leaning on `IdeamLinguistics`, `IdeamSplines` (prosody curves),
 * `IdeamMinds` (belief & misreference recording), and `IdeamGames` (RPG skills).
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/communications_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamCommunicationsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamCommunicationsPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamCommunicationsPlugin *singleton;

    static constexpr std::string_view COMMUNICATIONS_SETTINGS_PATH =
        "res://addons/ideam_communications/resources/project_communications_settings.res";

    godot::Ref<CommunicationsInspector> communications_inspector_;

public:
    IdeamCommunicationsPlugin();
    ~IdeamCommunicationsPlugin() override;

    [[nodiscard]] static IdeamCommunicationsPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    [[nodiscard]] godot::StringName get_plugin_domain_name() const {
        return godot::StringName("IdeamCommunications");
    }

    [[nodiscard]] bool is_linguistics_handshake_available() const { return is_plugin_active("IdeamLinguistics"); }
    [[nodiscard]] bool is_minds_handshake_available() const { return is_plugin_active("IdeamMinds"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_narratives_handshake_available() const { return is_plugin_active("IdeamNarratives"); }
    [[nodiscard]] bool is_games_handshake_available() const { return is_plugin_active("IdeamGames"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
