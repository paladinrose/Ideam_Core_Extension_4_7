#pragma once

/**
 * @file ideam_climatology_plugin.h
 * @brief IdeamEditorPlugin entry point for IdeamClimatology.
 *
 * Upgraded to inherit `IdeamEditorPlugin` for `.ideam_registry.cfg` handshakes,
 * shared `ToolContext` / `IdeamUndoRedo` wiring, and trans-environment gizmo support.
 */

#include "godot/climatology/climatology_weather_gizmo.h"
#include "godot/climatology/editor/climatology_inspector.h"
#include "godot/editor/ideam_editor_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class IdeamClimatologyPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamClimatologyPlugin, IdeamEditorPlugin)

private:
    static IdeamClimatologyPlugin *singleton;
    godot::Ref<ClimatologyInspector> inspector_plugin_;
    godot::Ref<ClimatologyWeatherGizmo> weather_gizmo_;

protected:
    static void _bind_methods();

public:
    IdeamClimatologyPlugin();
    ~IdeamClimatologyPlugin() override;

    [[nodiscard]] static IdeamClimatologyPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("ClimatologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedClimatology");
    }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
};

} // namespace ideam::godot_ext
