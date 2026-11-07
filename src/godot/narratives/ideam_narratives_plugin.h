#pragma once

#include "../editor/ideam_editor_plugin.h"

#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamNarrativesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamNarrativesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamNarrativesPlugin* singleton;

    // Ecosystem resource paths defining narratives layout defaults
    static constexpr std::string_view NARRATIVES_SETTINGS_PATH = "res://addons/ideam_narratives/resources/project_narratives_settings.res";

    // Helper functions
    void validate_narrative_script_templates();
    void _create_template(const godot::String& folder_name, const godot::String& default_file_name);

public:
    IdeamNarrativesPlugin();
    virtual ~IdeamNarrativesPlugin() override;

    static IdeamNarrativesPlugin* get_singleton() { return singleton; }

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;

    // --- Cross-Plugin Task Domain Handshake ---
    godot::StringName get_task_domain_name() const { return godot::StringName("IdeamNarratives"); }
    void register_task_domain_pack();

    // Static API exposed to Godot
    static godot::EditorUndoRedoManager* get_static_undo_redo();
};

} // namespace ideam::godot_ext