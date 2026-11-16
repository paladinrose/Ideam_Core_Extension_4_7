#pragma once

/**
 * @file ideam_climatology_plugin.h
 * @brief EditorPlugin entry point for IdeamClimatology.
 *
 * Registers ClimatologyInspector and verifies the IdeamTasks domain handshake.
 * Note: Cross-plugin bridges outside of IdeamTasks are intentionally held off per plan.
 */

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "godot/climatology/editor/climatology_inspector.h"

namespace ideam::godot_ext {

class IdeamClimatologyPlugin : public godot::EditorPlugin {
    GDCLASS(IdeamClimatologyPlugin, godot::EditorPlugin)

private:
    godot::Ref<ClimatologyInspector> inspector_plugin_;

protected:
    static void _bind_methods();

public:
    IdeamClimatologyPlugin() = default;
    ~IdeamClimatologyPlugin() override = default;

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("ClimatologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedClimatology");
    }
};

} // namespace ideam::godot_ext
