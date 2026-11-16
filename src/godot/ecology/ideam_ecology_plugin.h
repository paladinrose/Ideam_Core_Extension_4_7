#pragma once

/**
 * @file ideam_ecology_plugin.h
 * @brief EditorPlugin entry point for IdeamEcology.
 *
 * Registers EcologyInspector and verifies the IdeamTasks domain handshake.
 * Note: Cross-plugin bridges outside of IdeamTasks are intentionally held off per plan.
 */

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "godot/ecology/editor/ecology_inspector.h"

namespace ideam::godot_ext {

class IdeamEcologyPlugin : public godot::EditorPlugin {
    GDCLASS(IdeamEcologyPlugin, godot::EditorPlugin)

private:
    godot::Ref<EcologyInspector> inspector_plugin_;

protected:
    static void _bind_methods();

public:
    IdeamEcologyPlugin() = default;
    ~IdeamEcologyPlugin() override = default;

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("EcologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedEcology");
    }
};

} // namespace ideam::godot_ext
