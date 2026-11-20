#pragma once

/**
 * ideam_project_plugin.h
 * EditorPlugin entry point for the refactored Tier 2 Ideam Project Tools (Phase 2).
 *
 * Responsibilities:
 *   - Announces `IdeamProjectTools` active in `.ideam_registry.cfg`.
 *   - Registers `ProjectBlueprintEditor` Tier 1 controls for `ProjectBlueprintResource` and
 *     `ProjectModuleManifest` into `IdeamTier1ControlRegistry` (`ProjectWorkspace::WORKSPACE_ID`).
 *   - Exposes `Ideam/Project Workspace` in Godot's Project -> Tools menu (and cleanly removes it).
 *   - Listens to `SceneTree` `node_added` / `node_removed` for companion `IdeamEditorPlugin`
 *     lifecycle events (matching `IdeamTasksPlugin`) so enabling/disabling plugins live-syncs.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif

namespace ideam::godot_ext {

class IdeamProjectPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamProjectPlugin, IdeamEditorPlugin)

private:
    static IdeamProjectPlugin *singleton_;

protected:
    static void _bind_methods();

public:
    IdeamProjectPlugin();
    ~IdeamProjectPlugin() override;

    void _enter_tree() override;
    void _exit_tree() override;

    void open_project_workspace();
    void _on_tree_node_added(godot::Node *p_node);
    void _on_tree_node_removed(godot::Node *p_node);

    static IdeamProjectPlugin *get_singleton() { return singleton_; }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
