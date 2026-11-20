#include "ideam_project_plugin.h"

#include "ideam_project_contribution_registry.h"
#include "project_blueprint_resource.h"
#include "ui/project_blueprint_editor.h"
#include "ui/project_workspace.h"
#include "../tools/tool_registry.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamProjectPlugin *IdeamProjectPlugin::singleton_ = nullptr;

void IdeamProjectPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open_project_workspace"), &IdeamProjectPlugin::open_project_workspace);
    ClassDB::bind_method(D_METHOD("_on_tree_node_added", "node"), &IdeamProjectPlugin::_on_tree_node_added);
    ClassDB::bind_method(D_METHOD("_on_tree_node_removed", "node"), &IdeamProjectPlugin::_on_tree_node_removed);

#ifdef TOOLS_ENABLED
    ClassDB::bind_static_method(
        "IdeamProjectPlugin",
        D_METHOD("undo_redo"),
        &IdeamProjectPlugin::get_static_undo_redo
    );
#endif
    ClassDB::bind_static_method(
        "IdeamProjectPlugin",
        D_METHOD("create_undo_redo_wrapper"),
        &IdeamProjectPlugin::create_undo_redo_wrapper
    );
}

IdeamProjectPlugin::IdeamProjectPlugin() {
    if (singleton_ == nullptr) {
        singleton_ = this;
    }
}

IdeamProjectPlugin::~IdeamProjectPlugin() {
    if (singleton_ == this) {
        singleton_ = nullptr;
    }
}

void IdeamProjectPlugin::_enter_tree() {
    validate_script_templates();

    // 1. Ecosystem handshake
    set_plugin_active("IdeamProjectTools", true);

    // 2. Register Tier 1 Blueprint & Manifest editors for ProjectWorkspace
    auto &reg = ToolRegistry::get_singleton();
    const StringName ws_id(ProjectWorkspace::WORKSPACE_ID);

    reg.register_control({
        ws_id,
        "ProjectBlueprintResource",
        "Blueprints",
        "Project Blueprint",
        UPDATE_EXPLICIT_TRIGGER,
        [](const Ref<Resource> &p_asset, const Ref<ToolContext> &p_ctx) -> Control * {
            Ref<ProjectBlueprintResource> bp = p_asset;
            if (bp.is_null()) return nullptr;
            auto *ed = memnew(ProjectBlueprintEditor);
            ed->set_tool_context(p_ctx);
            ed->set_blueprint(bp);
            return ed;
        },
        [](Control *p_ctrl, const Ref<Resource> &p_asset, const Ref<ToolContext> &p_ctx) {
            if (auto *ed = Object::cast_to<ProjectBlueprintEditor>(p_ctrl)) {
                ed->set_tool_context(p_ctx);
                ed->set_blueprint(p_asset);
            }
        }
    });

    reg.register_control({
        ws_id,
        "ProjectModuleManifest",
        "Modules",
        "Module Manifest",
        UPDATE_ON_COMMIT,
        [](const Ref<Resource> &p_asset, const Ref<ToolContext> &p_ctx) -> Control * {
            Ref<ProjectModuleManifest> mod = p_asset;
            if (mod.is_null()) return nullptr;
            auto *ed = memnew(ProjectBlueprintEditor);
            ed->set_tool_context(p_ctx);
            ed->set_manifest(mod);
            return ed;
        },
        [](Control *p_ctrl, const Ref<Resource> &p_asset, const Ref<ToolContext> &p_ctx) {
            if (auto *ed = Object::cast_to<ProjectBlueprintEditor>(p_ctrl)) {
                ed->set_tool_context(p_ctx);
                ed->set_manifest(p_asset);
            }
        }
    });

    // 3. Register Tool Menu Entry
    add_tool_menu_item("Ideam/Project Workspace", Callable(this, "open_project_workspace"));

    // 4. Listen for companion IdeamEditorPlugin activation/deactivation in SceneTree
    if (SceneTree *tree = get_tree()) {
        if (!tree->is_connected("node_added", Callable(this, "_on_tree_node_added"))) {
            tree->connect("node_added", Callable(this, "_on_tree_node_added"));
        }
        if (!tree->is_connected("node_removed", Callable(this, "_on_tree_node_removed"))) {
            tree->connect("node_removed", Callable(this, "_on_tree_node_removed"));
        }
    }
}

void IdeamProjectPlugin::_exit_tree() {
    remove_tool_menu_item("Ideam/Project Workspace");

    if (SceneTree *tree = get_tree()) {
        if (tree->is_connected("node_added", Callable(this, "_on_tree_node_added"))) {
            tree->disconnect("node_added", Callable(this, "_on_tree_node_added"));
        }
        if (tree->is_connected("node_removed", Callable(this, "_on_tree_node_removed"))) {
            tree->disconnect("node_removed", Callable(this, "_on_tree_node_removed"));
        }
    }

    auto &reg = ToolRegistry::get_singleton();
    const StringName ws_id(ProjectWorkspace::WORKSPACE_ID);
    reg.unregister_class(ws_id, "ProjectBlueprintResource");
    reg.unregister_class(ws_id, "ProjectModuleManifest");

    ProjectWorkspace::close_shared_window();
    set_plugin_active("IdeamProjectTools", false);
}

void IdeamProjectPlugin::open_project_workspace() {
    if (ProjectWorkspace *ws = ProjectWorkspace::get_or_create_shared_workspace()) {
        if (ws->get_active_asset().is_null()) {
            ws->discover_and_open_active_blueprint();
        }
    }
}

void IdeamProjectPlugin::_on_tree_node_added(Node *p_node) {
    if (!p_node || p_node == this) {
        return;
    }
    if (Object::cast_to<IdeamEditorPlugin>(p_node)) {
        // Companion Ideam plugin enabled; next discovery pass picks up its manifests & hooks
    }
}

void IdeamProjectPlugin::_on_tree_node_removed(Node *p_node) {
    if (!p_node || p_node == this) {
        return;
    }
    if (Object::cast_to<IdeamEditorPlugin>(p_node)) {
        // Companion Ideam plugin disabled; registry state updated by that plugin's _exit_tree
    }
}

#ifdef TOOLS_ENABLED
EditorUndoRedoManager *IdeamProjectPlugin::get_static_undo_redo() {
    return singleton_ ? singleton_->get_undo_redo() : nullptr;
}
#endif

Ref<IdeamUndoRedo> IdeamProjectPlugin::create_undo_redo_wrapper() {
    Ref<IdeamUndoRedo> ur;
    ur.instantiate();
#ifdef TOOLS_ENABLED
    if (EditorUndoRedoManager *ed_ur = get_static_undo_redo()) {
        ur->set_editor_undo_redo(ed_ur);
    }
#endif
    return ur;
}

} // namespace ideam::godot_ext
