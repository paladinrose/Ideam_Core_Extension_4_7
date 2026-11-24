#pragma once

/**
 * @file ideam_geomorphology_plugin.h
 * @brief IdeamEditorPlugin entry point for IdeamGeomorphology.
 *
 * Upgraded to inherit `IdeamEditorPlugin` for `.ideam_registry.cfg` handshakes,
 * shared `ToolContext` / `IdeamUndoRedo` wiring, and trans-environment gizmo support.
 */

#include "godot/editor/ideam_editor_plugin.h"
#include "godot/geomorphology/editor/geomorphology_inspector.h"
#include "godot/geomorphology/geomorphology_terrain_gizmo.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class IdeamGeomorphologyPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamGeomorphologyPlugin, IdeamEditorPlugin)

private:
    static IdeamGeomorphologyPlugin *singleton;
    godot::Ref<GeomorphologyInspector> inspector_plugin_;
    godot::Ref<GeomorphologyTerrainGizmo> terrain_gizmo_;

protected:
    static void _bind_methods();

public:
    IdeamGeomorphologyPlugin();
    ~IdeamGeomorphologyPlugin() override;

    [[nodiscard]] static IdeamGeomorphologyPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("GeomorphologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedGeomorphology");
    }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
};

} // namespace ideam::godot_ext
