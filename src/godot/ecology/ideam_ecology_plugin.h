#pragma once

/**
 * @file ideam_ecology_plugin.h
 * @brief IdeamEditorPlugin entry point for IdeamEcology.
 *
 * Upgraded to inherit `IdeamEditorPlugin` for `.ideam_registry.cfg` handshakes,
 * shared `ToolContext` / `IdeamUndoRedo` wiring, and trans-environment gizmo support.
 */

#include "godot/ecology/ecology_biosphere_gizmo.h"
#include "godot/ecology/editor/ecology_inspector.h"
#include "godot/editor/ideam_editor_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class IdeamEcologyPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamEcologyPlugin, IdeamEditorPlugin)

private:
    static IdeamEcologyPlugin *singleton;
    godot::Ref<EcologyInspector> inspector_plugin_;
    godot::Ref<EcologyBiosphereGizmo> biosphere_gizmo_;

protected:
    static void _bind_methods();

public:
    IdeamEcologyPlugin();
    ~IdeamEcologyPlugin() override;

    [[nodiscard]] static IdeamEcologyPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    static bool is_tasks_handshake_available() {
        return godot::ClassDB::class_exists("EcologyTaskHost") &&
               godot::ClassDB::class_exists("DataOrientedEcology");
    }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
};

} // namespace ideam::godot_ext
