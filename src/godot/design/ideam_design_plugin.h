#pragma once

#include "../editor/ideam_editor_plugin.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/design_inspector.h"
#include "form_scaffold_node_3d.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamDesignPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamDesignPlugin, IdeamEditorPlugin)

private:
    static IdeamDesignPlugin *singleton;
    static constexpr std::string_view DESIGN_SETTINGS_PATH =
        "res://addons/ideam_design/resources/project_design_settings.res";
    static constexpr const char *TOOLS_MENU_COMPOSER_ITEM = "Ideam/Design Composer";

    godot::Ref<DesignInspector> inspector_plugin_;
    godot::Ref<FormScaffoldGizmo> form_scaffold_gizmo_;

    void _register_tools_and_workspace();
    void _unregister_tools_and_workspace();

protected:
    static void _bind_methods();

public:
    IdeamDesignPlugin();
    ~IdeamDesignPlugin() override;

    [[nodiscard]] static IdeamDesignPlugin *get_singleton() noexcept { return singleton; }

    void _enter_tree() override;
    void _exit_tree() override;

    void open_design_composer();

    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamDesign"); }

    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_surfaces_handshake_available() const { return is_plugin_active("IdeamSurfaces"); }
    [[nodiscard]] bool is_characters_handshake_available() const { return is_plugin_active("IdeamCharacters"); }
    [[nodiscard]] bool is_tiles_handshake_available() const { return is_plugin_active("IdeamTiles"); }
    [[nodiscard]] bool is_linguistics_handshake_available() const { return is_plugin_active("IdeamLinguistics"); }
    [[nodiscard]] bool is_tutorials_handshake_available() const { return is_plugin_active("IdeamTutorials"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
