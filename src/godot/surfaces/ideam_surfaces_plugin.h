#pragma once

/**
 * ideam_surfaces_plugin.h
 * Editor plugin entry point for the `IdeamSurfaces` module (`src/godot/surfaces/`).
 *
 * Architected directly on the `IdeamNarrativesPlugin` blueprint:
 *   - Manages the `"IdeamSurfaces"` active-plugin handshake in `.ideam_registry.cfg`.
 *   - Registers `SurfacesComposer` (`ToolWorkspace`), Tier 1 `SurfacePatchControl` factories,
 *     and `ToolDescriptor` catalog entries in `ToolRegistry` for `IdeamTutorials` & `AgentToolBridge`.
 *   - Registers `SurfacesInspector` (`IdeamEditorInspectorPlugin`) and `SurfacePatchGizmo`
 *     (`EditorNode3DGizmoPlugin`) during `_enter_tree()` and cleans them up in `_exit_tree()`.
 *   - Exposes static `EditorUndoRedoManager` and `IdeamUndoRedo` helpers and lateral handshakes
 *     to `IdeamSplines`, `IdeamGeometry`, `IdeamDesign`, `IdeamCharacters`, `IdeamTutorials`, and `IdeamTasks`.
 */

#include "../editor/ideam_editor_plugin.h"
#include "../gizmos/ideam_spatial_toolbar.h"
#include "../utilities/ideam_undo_redo.h"
#include "editor/surfaces_inspector.h"
#include "surface_mesh_node_3d.h"

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#endif
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <string_view>

namespace ideam::godot_ext {

class IdeamSurfacesPlugin : public IdeamEditorPlugin {
    GDCLASS(IdeamSurfacesPlugin, IdeamEditorPlugin)

protected:
    static void _bind_methods();

private:
    static IdeamSurfacesPlugin *singleton;

    static constexpr std::string_view SURFACES_SETTINGS_PATH =
        "res://addons/ideam_surfaces/resources/project_surfaces_settings.res";
    static constexpr const char *TOOLS_MENU_COMPOSER_ITEM = "Ideam/Surfaces Composer";

    godot::Ref<SurfacesInspector> surfaces_inspector_;
    godot::Ref<SurfacePatchGizmo> surface_patch_gizmo_;
    IdeamSpatialToolbar *spatial_toolbar_ = nullptr;
    uint64_t selected_surface_id_         = 0u;
    void _on_surface_mode_changed(const godot::StringName &p_mode_id);

    void _register_tools_and_workspace();
    void _unregister_tools_and_workspace();

public:
    IdeamSurfacesPlugin();
    ~IdeamSurfacesPlugin() override;

    [[nodiscard]] static IdeamSurfacesPlugin *get_singleton() noexcept { return singleton; }

    // Lifecycle
    void _enter_tree() override;
    void _exit_tree() override;
    bool _handles(godot::Object *p_object) const override;
    void _edit(godot::Object *p_object) override;
    void _make_visible(bool p_visible) override;

    // Menu callback
    void open_surfaces_composer();

    // --- Cross-Plugin Core & Lateral Handshakes ---
    [[nodiscard]] godot::StringName get_plugin_domain_name() const { return godot::StringName("IdeamSurfaces"); }
    [[nodiscard]] bool is_splines_handshake_available() const { return is_plugin_active("IdeamSplines"); }
    [[nodiscard]] bool is_geometry_handshake_available() const { return is_plugin_active("IdeamGeometry"); }
    [[nodiscard]] bool is_design_handshake_available() const { return is_plugin_active("IdeamDesign"); }
    [[nodiscard]] bool is_characters_handshake_available() const { return is_plugin_active("IdeamCharacters"); }
    [[nodiscard]] bool is_tutorials_handshake_available() const { return is_plugin_active("IdeamTutorials"); }
    [[nodiscard]] bool is_tasks_handshake_available() const { return is_plugin_active("IdeamTasks"); }

    // Static Undo / Redo API exposed to Godot & Surfaces DomainTools
#ifdef TOOLS_ENABLED
    static godot::EditorUndoRedoManager *get_static_undo_redo();
#endif
    static godot::Ref<IdeamUndoRedo> create_undo_redo_wrapper();
};

} // namespace ideam::godot_ext
