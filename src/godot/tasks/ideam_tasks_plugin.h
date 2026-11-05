#pragma once

#include "../editor/ideam_editor_plugin.h"
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamTasksPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamTasksPlugin, IdeamEditorPlugin)

private:
    static IdeamTasksPlugin *singleton;

    // Ecosystem resource paths defining tasks layout defaults
    static constexpr std::string_view TASKS_SETTINGS_PATH = "res://addons/ideam_tasks/resources/project_tasks_settings.res";

    // Inspector for Task resources (TaskGraphResource, etc.)
    godot::Ref<godot::EditorInspectorPlugin> task_inspector;
    
    // Status flag denoting structural safety
    bool manifest_valid = false;

    // Setup & companion plugin discovery helpers
    void _check_manifest_and_setup();
    void _discover_active_companion_plugins(godot::Node* p_root);
    void _try_mount_companion_plugin(godot::Node* p_node);
    void _try_unmount_companion_plugin(godot::Node* p_node);

protected:
    static void _bind_methods();

public:
    IdeamTasksPlugin();
    virtual ~IdeamTasksPlugin() override;

    static IdeamTasksPlugin* get_singleton() { return singleton; }

    // Lifecycle
    virtual void _enter_tree() override;
    virtual void _exit_tree() override;
    
    // Signal Callbacks
    void _on_manifest_updated();
    void _on_tree_node_added(godot::Node* p_node);
    void _on_tree_node_removed(godot::Node* p_node);

#ifdef TOOLS_ENABLED
    // Global Access
    static godot::EditorUndoRedoManager *undo_redo();
#endif
};

} // namespace ideam::godot_ext