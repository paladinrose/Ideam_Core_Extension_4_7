#include "ideam_tasks_plugin.h"
#include "task_graph_inspector.h"
#include "../../core/tasks/registration/ideam_task_registry.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

IdeamTasksPlugin *IdeamTasksPlugin::singleton = nullptr;

void IdeamTasksPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_manifest_updated"), &IdeamTasksPlugin::_on_manifest_updated);
    ClassDB::bind_method(D_METHOD("_on_tree_node_added", "node"), &IdeamTasksPlugin::_on_tree_node_added);
    ClassDB::bind_method(D_METHOD("_on_tree_node_removed", "node"), &IdeamTasksPlugin::_on_tree_node_removed);
}

IdeamTasksPlugin::IdeamTasksPlugin() {
    singleton = this;
}

IdeamTasksPlugin::~IdeamTasksPlugin() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

void IdeamTasksPlugin::_enter_tree() {
    validate_script_templates();
    
    // 1. Handshake: Announce the tasks plugin is active in the ecosystem
    set_plugin_active("IdeamTasks", true);

    // 2. Registry: Register the tasks scaffolding settings into the global Wizard ecosystem
    register_to_ecosystem("WizardSettings", "IdeamTasks", String(TASKS_SETTINGS_PATH.data()));

    // 3. Setup Task Registry Tools and UI
    auto registry = core::IdeamTaskRegistry::get_singleton();
    if (registry) {
        registry->sync_active_domains_from_config();

        if (!registry->is_connected("manifest_updated", Callable(this, "_on_manifest_updated"))) {
            registry->connect("manifest_updated", Callable(this, "_on_manifest_updated"));
        }
        
        add_tool_menu_item("Update Task Manifest", Callable(registry, "bake_manifest"));
    }

    // 4. Bidirectional Discovery: Scan already-enabled companion plugins in the SceneTree
    // and listen for future companion plugins being enabled or disabled.
    if (SceneTree* tree = get_tree()) {
        if (tree->get_root()) {
            _discover_active_companion_plugins(tree->get_root());
        }
        if (!tree->is_connected("node_added", Callable(this, "_on_tree_node_added"))) {
            tree->connect("node_added", Callable(this, "_on_tree_node_added"));
        }
        if (!tree->is_connected("node_removed", Callable(this, "_on_tree_node_removed"))) {
            tree->connect("node_removed", Callable(this, "_on_tree_node_removed"));
        }
    }

    _check_manifest_and_setup();
}

void IdeamTasksPlugin::_exit_tree() {
    remove_tool_menu_item("Update Task Manifest");

    if (SceneTree* tree = get_tree()) {
        if (tree->is_connected("node_added", Callable(this, "_on_tree_node_added"))) {
            tree->disconnect("node_added", Callable(this, "_on_tree_node_added"));
        }
        if (tree->is_connected("node_removed", Callable(this, "_on_tree_node_removed"))) {
            tree->disconnect("node_removed", Callable(this, "_on_tree_node_removed"));
        }
    }

    auto registry = core::IdeamTaskRegistry::get_singleton();
    if (registry && registry->is_connected("manifest_updated", Callable(this, "_on_manifest_updated"))) {
        registry->disconnect("manifest_updated", Callable(this, "_on_manifest_updated"));
    }

    // Clean teardown of the inspector plugin
    if (manifest_valid && task_inspector.is_valid()) {
        remove_inspector_plugin(task_inspector);
    }

    // Handshake: Remove plugin from active roster
    set_plugin_active("IdeamTasks", false);
}

void IdeamTasksPlugin::_discover_active_companion_plugins(Node* p_root) {
    if (!p_root) return;

    _try_mount_companion_plugin(p_root);

    int child_count = p_root->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        Node* child = p_root->get_child(i);
        // EditorPlugins live either directly under the root or one level down in EditorNode;
        // scanning EditorPlugin instances avoids traversing deep user scene trees.
        if (Object::cast_to<EditorPlugin>(child) || child->get_class() == "EditorNode" || p_root == get_tree()->get_root()) {
            _discover_active_companion_plugins(child);
        }
    }
}

void IdeamTasksPlugin::_try_mount_companion_plugin(Node* p_node) {
    if (!p_node || p_node == this) return;

    // Companion plugins that provide task manifests expose `register_task_domain_pack()`
    if (p_node->has_method("register_task_domain_pack")) {
        p_node->call("register_task_domain_pack");
    }
}

void IdeamTasksPlugin::_try_unmount_companion_plugin(Node* p_node) {
    if (!p_node || p_node == this) return;

    if (p_node->has_method("get_task_domain_name")) {
        StringName domain_name = p_node->call("get_task_domain_name");
        if (!domain_name.is_empty()) {
            if (auto* registry = core::IdeamTaskRegistry::get_singleton()) {
                registry->set_domain_pack_active(domain_name, false);
            }
        }
    }
}

void IdeamTasksPlugin::_on_tree_node_added(Node* p_node) {
    _try_mount_companion_plugin(p_node);
}

void IdeamTasksPlugin::_on_tree_node_removed(Node* p_node) {
    _try_unmount_companion_plugin(p_node);
}

void IdeamTasksPlugin::_on_manifest_updated() {
    _check_manifest_and_setup();
}

void IdeamTasksPlugin::_check_manifest_and_setup() {
    auto registry = core::IdeamTaskRegistry::get_singleton();
    
    if (registry && registry->are_active_manifests_valid()) {
        if (!manifest_valid) {
            manifest_valid = true;
            task_inspector = Ref<TaskGraphInspector>(memnew(TaskGraphInspector));
            add_inspector_plugin(task_inspector);
            UtilityFunctions::print("IdeamTasks: All active domain manifests validated. Task Graph Editing is enabled.");
        }
    } else {
        if (manifest_valid) {
            manifest_valid = false;
            remove_inspector_plugin(task_inspector);
            task_inspector.unref();
        }

        String outdated_list = "Core";
        if (registry) {
            PackedStringArray outdated = registry->get_outdated_active_domains();
            if (!outdated.is_empty()) {
                outdated_list = String(", ").join(outdated);
            }
        }

        UtilityFunctions::push_warning(
            "IdeamTasks: Task Manifest version mismatch or missing for domain(s): [",
            outdated_list,
            "]. Please go to Project > Tools > Update Task Manifest. Task Graph Editing is currently disabled."
        );
    }
}

#ifdef TOOLS_ENABLED
EditorUndoRedoManager *IdeamTasksPlugin::undo_redo() {
    if (singleton) {
        return singleton->get_undo_redo();
    }
    return nullptr;
}
#endif

} // namespace ideam::godot_ext