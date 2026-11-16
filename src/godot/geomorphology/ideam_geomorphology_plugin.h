#pragma once

/**
 * @file ideam_geomorphology_plugin.h
 * @brief EditorPlugin entry point for IdeamGeomorphology.
 *
 * Registers the GeomorphologyInspector and verifies the IdeamTasks domain handshake.
 * Note: Cross-plugin bridges outside of IdeamTasks are intentionally held off per plan.
 */

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "godot/geomorphology/editor/geomorphology_inspector.h"

namespace ideam::godot_ext {

class IdeamGeomorphologyPlugin : public godot::EditorPlugin {
    GDCLASS(IdeamGeomorphologyPlugin, godot::EditorPlugin)

private:
    godot::Ref<GeomorphologyInspector> inspector_plugin_;

protected:
    static void _bind_methods();

public:
    IdeamGeomorphologyPlugin() = default;
    ~IdeamGeomorphologyPlugin() override = default;

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("GeomorphologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedGeomorphology");
    }
};

} // namespace ideam::godot_ext
