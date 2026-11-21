#pragma once

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/narratives_inspector.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamNarrativesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamNarrativesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamNarrativesPlugin *singleton;

    static constexpr std::string_view NARRATIVES_SETTINGS_PATH =
        "res://addons/ideam_narratives/resources/project_narratives_settings.res";
    static constexpr const char *TOOLS_MENU_COMPOSER_ITEM = "Ideam/Narratives Composer";

    godot::Ref<NarrativesInspector> narratives_inspector_;

    // Helper functions
    void validate_narrative_script_templates();
    void _create_template(const godot::String &folder_name, const godot::String &default_file_name);
    void _register_tools_and_workspace();
    void _unregister_tools_and_workspace();

public:
    IdeamNarrativesPlugin();
    virtual ~IdeamNarrativesPlugin() override;

    static IdeamNarrativesPlugin *get_singleton() { return singleton; }

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // Menu callback
    void open_narratives_composer();

    // --- Cross-Plugin Core & Lateral Handshakes ---
    [[nodiscard]] godot::StringName get_task_domain_name() const { return godot::StringName("IdeamNarratives"); }
    void register_task_domain_pack();

    [[nodiscard]] bool is_linguistics_handshake_available() const { return is_plugin_active("IdeamLinguistics"); }
    [[nodiscard]] bool is_tutorials_handshake_available() const { return is_plugin_active("IdeamTutorials"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

    // Static Undo / Redo API exposed to Godot & Narratives DomainTools
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext