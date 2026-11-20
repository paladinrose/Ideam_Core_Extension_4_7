#pragma once

/**
 * ideam_cymatics_plugin.h
 * EditorPlugin entry point for `IdeamCymatics` (`src/godot/cymatics/`).
 */

#include "editor/cymatics_inspector.h"
#include "../editor/ideam_editor_plugin.h"

#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class IdeamCymaticsPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamCymaticsPlugin, IdeamEditorPlugin)

private:
    godot::Ref<CymaticsEditorInspectorPlugin> inspector_plugin_;

protected:
    static void _bind_methods();

public:
    IdeamCymaticsPlugin() = default;
    ~IdeamCymaticsPlugin() override = default;

    void _enter_tree() override;
    void _exit_tree() override;
};

} // namespace ideam::godot_ext
